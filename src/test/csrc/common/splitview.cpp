/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "splitview.h"
#include "common.h"

#ifndef CONFIG_SPLITVIEW

void common_splitview_preinit(const char *) {}

void common_splitview_finish() {}

void common_splitview_force_cleanup() {}

void common_splitview_request_finish() {}

void common_splitview_request_detach() {}

void common_splitview_set_uart_input_fd(int) {}

void common_splitview_set_log_path(const char *) {}

bool common_splitview_is_active() {
  return false;
}

int common_splitview_uart_fd() {
  return -1;
}

#else

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <deque>
#include <fcntl.h>
#include <iterator>
#include <mutex>
#include <poll.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <vector>

namespace {

constexpr size_t kMaxLines = 5000;
constexpr size_t kMaxWrappedSegments = 32768;
constexpr const char *kDefaultLogPath = "build/logs";
constexpr const char *kUartLogName = "uart.log";
constexpr const char *kHostLogName = "host.log";
constexpr const char *kAllLogName = "all.log";
constexpr auto kFinishDrainQuietTime = std::chrono::milliseconds(150);
constexpr std::string_view kExitHintText = "\033[1;35mPress Q/q to exit\033[0m\n";
constexpr const char *kAnsiReset = "\033[0m";
constexpr const char *kAnsiHeaderUart = "\033[1;92m";
constexpr const char *kAnsiHeaderLogs = "\033[1;38;5;214m";
constexpr const char *kTerminalDetachSequence =
    "\033[?9l\033[?1000l\033[?1001l\033[?1002l\033[?1003l\033[?1004l\033[?1005l\033[?1006l\033[?1007l"
    "\033[?1015l\033[?1016l\033[?2004l\033[0m\033[?25h\033[?1049l\033[?1048l\033[?1047l";

enum class EscapeState {
  None,
  Escape,
  Csi,
};

struct PaneBuffer {
  std::deque<std::string> lines = {""};
  EscapeState escape_state = EscapeState::None;
  std::string pending_escape;
  bool pending_carriage_return = false;
  int wrapped_cache_width = -1;
  bool wrapped_cache_valid = false;
  bool wrapped_cache_has_content = false;
  std::vector<std::string> wrapped_lines_cache;

  void invalidate_wrapped_cache() {
    wrapped_cache_width = -1;
    wrapped_cache_valid = false;
    wrapped_cache_has_content = false;
    wrapped_lines_cache.clear();
  }

  void push_line() {
    if (lines.size() >= kMaxLines) {
      lines.pop_front();
    }
    lines.emplace_back();
  }

  void append_char(char ch) {
    if (lines.empty()) {
      lines.emplace_back();
    }
    lines.back().push_back(ch);
  }

  void append_escape_char(char ch) {
    pending_escape.push_back(ch);
  }

  void flush_pending_escape() {
    if (pending_escape.empty()) {
      return;
    }
    if (lines.empty()) {
      lines.emplace_back();
    }
    lines.back().append(pending_escape);
    pending_escape.clear();
    escape_state = EscapeState::None;
  }

  void feed(std::string_view chunk) {
    invalidate_wrapped_cache();
    for (unsigned char ch: chunk) {
      if (pending_carriage_return) {
        flush_pending_escape();
        if (ch == '\n') {
          push_line();
          pending_carriage_return = false;
          continue;
        }
        push_line();
        pending_carriage_return = false;
      }

      if (escape_state == EscapeState::Escape) {
        append_escape_char(static_cast<char>(ch));
        if (ch == '[') {
          escape_state = EscapeState::Csi;
        } else {
          flush_pending_escape();
        }
        continue;
      }

      if (escape_state == EscapeState::Csi) {
        append_escape_char(static_cast<char>(ch));
        if (ch >= '@' && ch <= '~') {
          flush_pending_escape();
        }
        continue;
      }

      if (ch == '\x1b') {
        pending_escape.clear();
        append_escape_char(static_cast<char>(ch));
        escape_state = EscapeState::Escape;
        continue;
      }

      switch (ch) {
        case '\r':
          flush_pending_escape();
          pending_carriage_return = true;
          continue;
        case '\n':
          flush_pending_escape();
          push_line();
          continue;
        case '\t':
          append_char(' ');
          append_char(' ');
          append_char(' ');
          append_char(' ');
          continue;
        case '\b':
          if (!lines.empty() && !lines.back().empty()) {
            lines.back().pop_back();
          }
          continue;
        case '\0': continue;
        default: append_char(static_cast<char>(ch)); continue;
      }
    }
  }

  void ensure_wrapped_cache(int width);

  int wrapped_line_count(int width) {
    ensure_wrapped_cache(width);
    return static_cast<int>(wrapped_lines_cache.size());
  }
};

struct ScreenSize {
  int rows = 24;
  int cols = 80;
};

constexpr int kWheelScrollLines = 3;

enum class PaneId {
  Uart,
  Logs,
};

struct SplitLayout {
  int cols = 80;
  int rows = 24;
  int left_width = 40;
  int right_width = 39;
  int body_rows = 22;
};

struct PaneRenderRange {
  int start = 0;
  int visible_lines = 0;
  bool has_content = false;
};

struct PaneTarget {
  PaneBuffer *buffer = nullptr;
  int *offset = nullptr;
  int width = 0;
};

struct ScrollState {
  int uart_offset = 0;
  int log_offset = 0;
  PaneId focused = PaneId::Logs;
};

struct InputEvent {
  enum class Type {
    Quit,
    Interrupt,
    MouseWheel,
  } type;
  int amount = 0;
  int mouse_x = 0;
  int mouse_y = 0;
};

struct NativeSplitViewRuntime {
  std::atomic<bool> initialized = false;
  std::atomic<bool> active = false;
  std::atomic<bool> stopping = false;
  std::atomic<bool> finish_requested = false;
  std::atomic<bool> finished = false;
  std::atomic<bool> detach_requested = false;
  std::atomic<bool> resize_requested = false;
  std::atomic<bool> input_mode_enabled = false;
  std::atomic<bool> input_capture_released = false;
  std::atomic<bool> force_cleanup_started = false;
  int tty_fd = -1;
  int input_fd = -1;
  int saved_stdout = -1;
  int saved_stderr = -1;
  int control_pipe_read = -1;
  int control_pipe_write = -1;
  int log_pipe_read = -1;
  int uart_pipe_read = -1;
  int uart_pipe_write = -1;
  std::atomic<int> uart_input_fd = -1;
  struct termios saved_input_termios {};
  std::thread render_thread;
};

NativeSplitViewRuntime g_runtime;
std::string g_splitview_log_path = kDefaultLogPath;

void handle_sigwinch(int) {
  g_runtime.resize_requested = true;
}

bool splitview_env_disabled() {
  const char *env = getenv("DIFFTEST_SPLITVIEW");
  return env != nullptr && std::strcmp(env, "0") == 0;
}

bool should_enable_splitview(const char *program_name) {
  if (splitview_env_disabled()) {
    return false;
  }
  if (program_name != nullptr && std::strcmp(program_name, "simv") == 0) {
    return false;
  }
  const char *term = getenv("TERM");
  if (term != nullptr && std::strcmp(term, "dumb") == 0) {
    return false;
  }
  return isatty(STDOUT_FILENO);
}

void close_if_open(int &fd) {
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
}

void close_uart_input_fd() {
  const int fd = g_runtime.uart_input_fd.exchange(-1);
  if (fd >= 0) {
    close(fd);
  }
}

void cleanup_pipe_array(std::array<int, 2> &pipe_fds) {
  close_if_open(pipe_fds[0]);
  close_if_open(pipe_fds[1]);
}

void maybe_start_external_uart_input() {
  if (!g_runtime.active) {
    return;
  }

  const char *path = getenv("DIFFTEST_SPLITVIEW_UART_INPUT");
  if (path == nullptr || path[0] == '\0') {
    return;
  }

  int fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    return;
  }

  close_if_open(g_runtime.uart_pipe_read);
  g_runtime.uart_pipe_read = fd;
  close_if_open(g_runtime.uart_pipe_write);
}

void write_all(int fd, const char *data, size_t len) {
  while (len > 0) {
    ssize_t written = write(fd, data, len);
    if (written > 0) {
      data += written;
      len -= static_cast<size_t>(written);
      continue;
    }
    if (written < 0 && errno == EINTR) {
      continue;
    }
    return;
  }
}

void write_all(int fd, const std::string &text) {
  write_all(fd, text.data(), text.size());
}

bool ensure_directory(const std::string &path) {
  if (path.empty()) {
    return false;
  }

  std::string current;
  size_t pos = 0;
  if (path[0] == '/') {
    current = "/";
    pos = 1;
  }

  while (pos <= path.size()) {
    size_t next = path.find('/', pos);
    std::string part = path.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
    if (!part.empty() && part != ".") {
      if (!current.empty() && current.back() != '/') {
        current.push_back('/');
      }
      current += part;
      if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
      }
    }
    if (next == std::string::npos) {
      break;
    }
    pos = next + 1;
  }

  return true;
}

std::string join_log_path(const std::string &dir, const char *name) {
  if (dir.empty() || dir == ".") {
    return name;
  }
  if (dir.back() == '/') {
    return dir + name;
  }
  return dir + "/" + name;
}

struct SplitLogPaths {
  std::string uart;
  std::string host;
  std::string all;
};

SplitLogPaths make_split_log_paths() {
  const std::string dir = g_splitview_log_path.empty() ? kDefaultLogPath : g_splitview_log_path;
  ensure_directory(dir);
  return {join_log_path(dir, kUartLogName), join_log_path(dir, kHostLogName), join_log_path(dir, kAllLogName)};
}

class AsyncFileWriter {
public:
  AsyncFileWriter() = default;
  AsyncFileWriter(const AsyncFileWriter &) = delete;
  AsyncFileWriter &operator=(const AsyncFileWriter &) = delete;

  bool start(const char *path) {
    fd_ = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd_ < 0) {
      return false;
    }
    worker_ = std::thread(&AsyncFileWriter::run, this);
    return true;
  }

  void enqueue(std::string_view chunk) {
    if (chunk.empty() || fd_ < 0) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closing_) {
        return;
      }
      queue_.emplace_back(chunk.data(), chunk.size());
    }
    cv_.notify_one();
  }

  void finish() {
    if (fd_ < 0) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      closing_ = true;
    }
    cv_.notify_one();

    if (worker_.joinable()) {
      worker_.join();
    }

    close_if_open(fd_);
  }

  ~AsyncFileWriter() {
    finish();
  }

private:
  void run() {
    while (true) {
      std::deque<std::string> pending;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return closing_ || !queue_.empty(); });
        if (queue_.empty() && closing_) {
          break;
        }
        pending.swap(queue_);
      }

      for (const std::string &chunk: pending) {
        write_all(fd_, chunk);
      }
    }
  }

  int fd_ = -1;
  bool closing_ = false;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<std::string> queue_;
  std::thread worker_;
};

ScreenSize query_screen_size(int tty_fd) {
  struct winsize ws {};
  if (ioctl(tty_fd, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
    return {static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
  }
  return {};
}

std::string fit_text(std::string_view text, int width) {
  if (width <= 0) {
    return "";
  }
  std::string out;
  out.reserve(static_cast<size_t>(width));
  for (char ch: text) {
    if (static_cast<int>(out.size()) == width) {
      break;
    }
    out.push_back(ch);
  }
  if (static_cast<int>(out.size()) < width) {
    out.append(static_cast<size_t>(width - static_cast<int>(out.size())), ' ');
  }
  return out;
}

bool is_ansi_sequence_final(unsigned char ch) {
  return ch >= '@' && ch <= '~';
}

bool is_sgr_sequence(std::string_view seq) {
  return seq.size() >= 2 && seq[0] == '\x1b' && seq[1] == '[' && !seq.empty() && seq.back() == 'm';
}

bool is_sgr_reset(std::string_view seq) {
  if (!is_sgr_sequence(seq)) {
    return false;
  }
  const std::string_view body = seq.substr(2, seq.size() - 3);
  return body.empty() || body == "0";
}

size_t next_ansi_sequence_end(std::string_view line, size_t pos) {
  if (pos >= line.size() || line[pos] != '\x1b') {
    return pos;
  }

  size_t i = pos + 1;
  if (i < line.size() && line[i] == '[') {
    ++i;
    while (i < line.size()) {
      if (is_ansi_sequence_final(static_cast<unsigned char>(line[i]))) {
        return i + 1;
      }
      ++i;
    }
    return line.size();
  }
  return std::min(pos + 2, line.size());
}

std::vector<std::string> wrap_ansi_line(const std::string &line, int width) {
  std::vector<std::string> wrapped;
  if (width <= 0) {
    return wrapped;
  }

  std::string current;
  std::string active_sgr_state;
  int visible = 0;

  auto finish_segment = [&](bool allow_empty) {
    if (!allow_empty && visible == 0 && current.empty()) {
      return;
    }

    std::string segment = current;
    if (!active_sgr_state.empty()) {
      segment += kAnsiReset;
    }
    if (visible < width) {
      segment.append(static_cast<size_t>(width - visible), ' ');
    }
    wrapped.push_back(std::move(segment));

    current.clear();
    if (!active_sgr_state.empty()) {
      current += active_sgr_state;
    }
    visible = 0;
  };

  size_t pos = 0;
  while (pos < line.size()) {
    if (line[pos] == '\x1b') {
      size_t end = next_ansi_sequence_end(line, pos);
      std::string seq = line.substr(pos, end - pos);
      current += seq;
      if (is_sgr_sequence(seq)) {
        if (is_sgr_reset(seq)) {
          active_sgr_state.clear();
        } else {
          active_sgr_state += seq;
        }
      }
      pos = end;
      continue;
    }

    if (visible == width) {
      finish_segment(false);
    }
    current.push_back(line[pos]);
    ++visible;
    ++pos;
  }

  if (visible == 0 && wrapped.empty()) {
    wrapped.emplace_back(width, ' ');
    return wrapped;
  }

  finish_segment(true);
  return wrapped;
}

void PaneBuffer::ensure_wrapped_cache(int width) {
  if (width <= 0) {
    wrapped_lines_cache.clear();
    wrapped_cache_width = width;
    wrapped_cache_valid = true;
    wrapped_cache_has_content = false;
    return;
  }

  if (wrapped_cache_valid && wrapped_cache_width == width) {
    return;
  }

  wrapped_lines_cache.clear();
  wrapped_cache_has_content = false;
  wrapped_lines_cache.reserve(lines.size());
  for (const std::string &line: lines) {
    wrapped_cache_has_content = wrapped_cache_has_content || !line.empty();
    std::vector<std::string> wrapped = wrap_ansi_line(line, width);
    wrapped_lines_cache.insert(wrapped_lines_cache.end(), std::make_move_iterator(wrapped.begin()),
                               std::make_move_iterator(wrapped.end()));
  }

  if (wrapped_lines_cache.size() > kMaxWrappedSegments) {
    wrapped_lines_cache.erase(wrapped_lines_cache.begin(),
                              wrapped_lines_cache.end() - static_cast<std::ptrdiff_t>(kMaxWrappedSegments));
  }

  if (wrapped_lines_cache.empty()) {
    wrapped_lines_cache.emplace_back(static_cast<size_t>(width), ' ');
  }

  wrapped_cache_width = width;
  wrapped_cache_valid = true;
}

SplitLayout compute_layout(const ScreenSize &screen) {
  SplitLayout layout;
  layout.cols = std::max(screen.cols, 20);
  layout.rows = std::max(screen.rows, 4);

  const int divider_width = 1;
  const int usable_cols = std::max(2, layout.cols - divider_width);
  layout.left_width = std::max(1, (usable_cols + 1) / 2);
  layout.right_width = std::max(1, usable_cols - layout.left_width);
  layout.body_rows = std::max(1, layout.rows - 2);
  return layout;
}

int clamp_scroll_offset(int offset, int total_lines, int height) {
  return std::clamp(offset, 0, std::max(0, total_lines - height));
}

PaneRenderRange describe_pane(PaneBuffer &pane, int width, int height, int scroll_offset) {
  PaneRenderRange range;
  if (width <= 0 || height <= 0) {
    return range;
  }

  pane.ensure_wrapped_cache(width);
  range.has_content = pane.wrapped_cache_has_content;
  const int total_lines = static_cast<int>(pane.wrapped_lines_cache.size());
  const int clamped_offset = clamp_scroll_offset(scroll_offset, total_lines, height);
  range.visible_lines = std::min(height, total_lines);
  const int end = total_lines - clamped_offset;
  range.start = std::max(0, end - range.visible_lines);
  return range;
}

bool pane_from_mouse_position(int mouse_x, int mouse_y, const SplitLayout &layout, PaneId &pane) {
  if (mouse_y < 1 || mouse_y > layout.body_rows + 1) {
    return false;
  }
  if (mouse_x >= 1 && mouse_x <= layout.left_width) {
    pane = PaneId::Uart;
    return true;
  }
  if (mouse_x >= layout.left_width + 2 && mouse_x <= layout.left_width + 1 + layout.right_width) {
    pane = PaneId::Logs;
    return true;
  }
  return false;
}

PaneTarget resolve_pane_target(PaneId pane, ScrollState &scroll, PaneBuffer &uart, PaneBuffer &logs,
                               const SplitLayout &layout) {
  if (pane == PaneId::Uart) {
    return PaneTarget{&uart, &scroll.uart_offset, layout.left_width};
  }
  return PaneTarget{&logs, &scroll.log_offset, layout.right_width};
}

std::string make_footer_text(const std::string &program_name, bool finished, bool input_enabled,
                             const ScrollState &scroll, int width) {
  std::string left = program_name;
  left += finished ? " finished" : " native split view";

  if (input_enabled) {
    left += " | mouse wheel scroll";
  }

  if (scroll.uart_offset > 0 || scroll.log_offset > 0) {
    left += " | UART v";
    left += std::to_string(scroll.uart_offset);
    left += " LOG v";
    left += std::to_string(scroll.log_offset);
  }

  return fit_text(left, width);
}

void draw_screen(int tty_fd, PaneBuffer &uart, PaneBuffer &logs, const SplitLayout &layout, const ScrollState &scroll,
                 const std::string &footer_text) {
  PaneRenderRange left = describe_pane(uart, layout.left_width, layout.body_rows, scroll.uart_offset);
  PaneRenderRange right = describe_pane(logs, layout.right_width, layout.body_rows, scroll.log_offset);
  std::string footer = fit_text(footer_text, layout.cols);
  std::string left_title = fit_text(std::string(scroll.focused == PaneId::Uart ? " *UART" : " UART") +
                                        (left.has_content ? "" : " [empty]") + " ",
                                    layout.left_width);
  std::string right_title =
      fit_text(std::string(scroll.focused == PaneId::Logs ? " *DIFFTEST / HOST LOG" : " DIFFTEST / HOST LOG") +
                   (right.has_content ? "" : " [empty]") + " ",
               layout.right_width);

  std::string screen_buf;
  screen_buf.reserve(static_cast<size_t>(layout.rows * (layout.cols + 1) + 32));
  screen_buf += "\033[H";
  screen_buf += kAnsiHeaderUart;
  screen_buf += left_title;
  screen_buf += kAnsiReset;
  screen_buf += '|';
  screen_buf += kAnsiHeaderLogs;
  screen_buf += right_title;
  screen_buf += kAnsiReset;
  screen_buf += '\n';

  for (int i = 0; i < layout.body_rows; ++i) {
    if (i < left.visible_lines) {
      screen_buf += uart.wrapped_lines_cache[static_cast<size_t>(left.start + i)];
    } else {
      screen_buf.append(static_cast<size_t>(layout.left_width), ' ');
    }

    screen_buf += '|';
    if (i < right.visible_lines) {
      screen_buf += logs.wrapped_lines_cache[static_cast<size_t>(right.start + i)];
    } else {
      screen_buf.append(static_cast<size_t>(layout.right_width), ' ');
    }
    screen_buf += '\n';
  }

  screen_buf += footer;
  if (static_cast<int>(footer.size()) < layout.cols) {
    screen_buf.append(static_cast<size_t>(layout.cols - static_cast<int>(footer.size())), ' ');
  }
  screen_buf += "\033[J";

  write_all(tty_fd, screen_buf);
}

bool enable_input_mode(int input_fd, struct termios &saved_termios) {
  if (input_fd < 0) {
    return false;
  }
  if (tcgetattr(input_fd, &saved_termios) != 0) {
    return false;
  }

  struct termios raw = saved_termios;
  raw.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO | ISIG));
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  return tcsetattr(input_fd, TCSANOW, &raw) == 0;
}

void flush_input_queue(int input_fd) {
  if (input_fd < 0) {
    return;
  }
  tcflush(input_fd, TCIFLUSH);
}

void drain_late_terminal_input(int input_fd, int timeout_ms);

void write_terminal_detach(int tty_fd) {
  if (tty_fd >= 0) {
    write_all(tty_fd, kTerminalDetachSequence, std::strlen(kTerminalDetachSequence));
  }
}

void restore_terminal_state(int tty_fd, int input_fd, const struct termios *saved_termios, bool have_saved_termios) {
  if (have_saved_termios && input_fd >= 0) {
    tcsetattr(input_fd, TCSAFLUSH, saved_termios);
  } else {
    flush_input_queue(input_fd);
  }
  write_terminal_detach(tty_fd);
  drain_late_terminal_input(input_fd, 20);
}

void drain_input_fd_once(int input_fd) {
  if (input_fd < 0) {
    return;
  }

  int flags = fcntl(input_fd, F_GETFL, 0);
  if (flags < 0) {
    return;
  }

  if ((flags & O_NONBLOCK) == 0) {
    fcntl(input_fd, F_SETFL, flags | O_NONBLOCK);
  }

  std::array<char, 256> discard{};
  while (read(input_fd, discard.data(), discard.size()) > 0) {}

  if ((flags & O_NONBLOCK) == 0) {
    fcntl(input_fd, F_SETFL, flags);
  }
}

void set_nonblocking(int fd) {
  if (fd < 0) {
    return;
  }

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags >= 0 && (flags & O_NONBLOCK) == 0) {
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
}

void drain_late_terminal_input(int input_fd, int timeout_ms) {
  if (input_fd < 0 || timeout_ms <= 0) {
    return;
  }

  constexpr int kPollSliceMs = 10;
  int remaining_ms = timeout_ms;
  while (remaining_ms > 0) {
    struct pollfd pfd {};
    pfd.fd = input_fd;
    pfd.events = POLLIN;
    int wait_ms = std::min(remaining_ms, kPollSliceMs);
    int rc = poll(&pfd, 1, wait_ms);
    if (rc <= 0) {
      remaining_ms -= wait_ms;
      continue;
    }
    if (pfd.revents & POLLIN) {
      drain_input_fd_once(input_fd);
      remaining_ms = timeout_ms;
      continue;
    }
    break;
  }
}

void restore_standard_streams() {
  if (g_runtime.saved_stdout >= 0) {
    dup2(g_runtime.saved_stdout, STDOUT_FILENO);
    close_if_open(g_runtime.saved_stdout);
  }
  if (g_runtime.saved_stderr >= 0) {
    dup2(g_runtime.saved_stderr, STDERR_FILENO);
    close_if_open(g_runtime.saved_stderr);
  }
}

void finalize_terminal_cleanup(bool input_enabled) {
  if (g_runtime.input_capture_released.exchange(true)) {
    g_runtime.input_mode_enabled = false;
    g_runtime.active = false;
    return;
  }

  close_if_open(g_runtime.uart_pipe_write);
  restore_terminal_state(g_runtime.tty_fd, g_runtime.input_fd, &g_runtime.saved_input_termios,
                         g_runtime.input_mode_enabled || input_enabled);
  g_runtime.input_mode_enabled = false;
  g_runtime.active = false;
}

void restore_input_mode(int input_fd, const struct termios &saved_termios, bool enabled) {
  if (enabled) {
    tcsetattr(input_fd, TCSAFLUSH, &saved_termios);
  } else {
    flush_input_queue(input_fd);
  }
}

void detach_splitview_runtime(bool input_enabled) {
  restore_standard_streams();
  finalize_terminal_cleanup(input_enabled);
}

enum class ParseResult {
  Parsed,
  Skipped,
  Incomplete,
};

ParseResult parse_input_event(std::string_view text, InputEvent &event, size_t &consumed) {
  consumed = 0;
  if (text.empty()) {
    return ParseResult::Incomplete;
  }

  if (text[0] != '\x1b') {
    consumed = 1;
    switch (text[0]) {
      case 'q':
      case 'Q': event.type = InputEvent::Type::Quit; return ParseResult::Parsed;
      case '\x03': event.type = InputEvent::Type::Interrupt; return ParseResult::Parsed;
      default: return ParseResult::Skipped;
    }
  }

  if (text.size() == 1) {
    return ParseResult::Incomplete;
  }

  if (text.rfind("\x1b[<", 0) == 0) {
    size_t end = 3;
    while (end < text.size() && text[end] != 'M' && text[end] != 'm') {
      ++end;
    }
    if (end >= text.size()) {
      return text.size() < 64 ? ParseResult::Incomplete : ParseResult::Skipped;
    }

    consumed = end + 1;
    int button = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    char suffix = '\0';
    std::string seq(text.substr(0, consumed));
    if (std::sscanf(seq.c_str(), "\x1b[<%d;%d;%d%c", &button, &mouse_x, &mouse_y, &suffix) != 4) {
      return ParseResult::Skipped;
    }
    if ((suffix != 'M' && suffix != 'm') || (button & 64) == 0) {
      return ParseResult::Skipped;
    }

    event.type = InputEvent::Type::MouseWheel;
    event.amount = (button & 1) == 0 ? kWheelScrollLines : -kWheelScrollLines;
    event.mouse_x = mouse_x;
    event.mouse_y = mouse_y;
    return ParseResult::Parsed;
  }

  if (text.rfind("\x1b[", 0) == 0 && text.size() < 8) {
    return ParseResult::Incomplete;
  }

  consumed = 1;
  return ParseResult::Skipped;
}

void consume_input(std::string &pending_input, std::string_view chunk, bool finished, std::vector<InputEvent> &events,
                   std::string &uart_input) {
  pending_input.append(chunk.data(), chunk.size());
  size_t offset = 0;
  while (offset < pending_input.size()) {
    InputEvent event{InputEvent::Type::Quit};
    size_t consumed = 0;
    ParseResult result = parse_input_event(std::string_view(pending_input).substr(offset), event, consumed);
    if (result == ParseResult::Incomplete) {
      break;
    }
    if (consumed == 0) {
      consumed = 1;
    }
    if (result == ParseResult::Parsed) {
      if (event.type == InputEvent::Type::Quit && !finished) {
        uart_input.append(pending_input.data() + offset, consumed);
      } else {
        events.push_back(event);
      }
    } else if (result == ParseResult::Skipped && !finished) {
      uart_input.append(pending_input.data() + offset, consumed);
    }
    offset += consumed;
  }

  pending_input.erase(0, offset);
  if (pending_input.size() > 128) {
    pending_input.clear();
  }
}

void adjust_scroll_for_appended_output(PaneBuffer &pane, int width, int &scroll_offset, std::string_view chunk) {
  if (scroll_offset <= 0 || width <= 0) {
    pane.feed(chunk);
    return;
  }

  const int before = pane.wrapped_line_count(width);
  pane.feed(chunk);
  const int after = pane.wrapped_line_count(width);
  scroll_offset = std::max(0, scroll_offset + after - before);
}

void append_exit_hint(PaneBuffer &logs, const SplitLayout &layout, ScrollState &scroll, bool &dirty,
                      bool &exit_hint_written) {
  if (exit_hint_written) {
    return;
  }
  std::string hint;
  if (!logs.lines.empty() && !logs.lines.back().empty()) {
    hint.push_back('\n');
  }
  hint += kExitHintText;
  scroll.log_offset = 0;
  adjust_scroll_for_appended_output(logs, layout.right_width, scroll.log_offset, hint);
  scroll.log_offset = 0;
  scroll.focused = PaneId::Logs;
  exit_hint_written = true;
  dirty = true;
}

void clamp_scroll_state(ScrollState &scroll, PaneBuffer &uart, PaneBuffer &logs, const SplitLayout &layout) {
  scroll.uart_offset =
      clamp_scroll_offset(scroll.uart_offset, uart.wrapped_line_count(layout.left_width), layout.body_rows);
  scroll.log_offset =
      clamp_scroll_offset(scroll.log_offset, logs.wrapped_line_count(layout.right_width), layout.body_rows);
}

void apply_scroll_delta(PaneId pane, int delta, ScrollState &scroll, PaneBuffer &uart, PaneBuffer &logs,
                        const SplitLayout &layout) {
  PaneTarget target = resolve_pane_target(pane, scroll, uart, logs, layout);
  const int total_lines = target.buffer->wrapped_line_count(target.width);
  *target.offset = clamp_scroll_offset(*target.offset + delta, total_lines, layout.body_rows);
}

bool drain_pipe_into_pane(int fd, short revents, PaneBuffer &pane, int width, int &scroll_offset,
                          AsyncFileWriter &file_writer, AsyncFileWriter &all_writer, bool &dirty, bool &saw_output) {
  if (fd < 0 || (revents & (POLLIN | POLLHUP)) == 0) {
    return true;
  }

  std::array<char, 512> buf{};
  bool pipe_open = true;
  while (true) {
    const ssize_t n = read(fd, buf.data(), buf.size());
    if (n > 0) {
      std::string_view chunk(buf.data(), static_cast<size_t>(n));
      file_writer.enqueue(chunk);
      all_writer.enqueue(chunk);
      adjust_scroll_for_appended_output(pane, width, scroll_offset, chunk);
      dirty = true;
      saw_output = true;
      continue;
    }
    if (n == 0) {
      pipe_open = false;
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
    pipe_open = false;
    break;
  }

  return pipe_open;
}

void handle_uart_input(std::string_view chunk, PaneBuffer &uart, const SplitLayout &layout, ScrollState &scroll,
                       bool &dirty) {
  const int uart_input_fd = g_runtime.uart_input_fd.load();
  if (uart_input_fd < 0 || chunk.empty()) {
    return;
  }

  write(uart_input_fd, chunk.data(), chunk.size());
  adjust_scroll_for_appended_output(uart, layout.left_width, scroll.uart_offset, chunk);
  dirty = true;
}

void handle_input_event(const InputEvent &event, ScrollState &scroll, PaneBuffer &uart, PaneBuffer &logs,
                        const SplitLayout &layout, bool &finish_pending,
                        std::chrono::steady_clock::time_point &finish_ready_at, bool finished, bool input_enabled,
                        bool &should_quit, bool &dirty) {
  switch (event.type) {
    case InputEvent::Type::Quit:
      if (finished) {
        should_quit = true;
        detach_splitview_runtime(input_enabled);
      }
      return;
    case InputEvent::Type::Interrupt:
      if (finished) {
        should_quit = true;
        detach_splitview_runtime(input_enabled);
        return;
      }
      signal_num = SIGINT;
      finish_pending = true;
      finish_ready_at = std::chrono::steady_clock::now() + kFinishDrainQuietTime;
      dirty = true;
      return;
    case InputEvent::Type::MouseWheel: {
      PaneId pane = scroll.focused;
      if (!pane_from_mouse_position(event.mouse_x, event.mouse_y, layout, pane)) {
        return;
      }
      scroll.focused = pane;
      apply_scroll_delta(pane, event.amount, scroll, uart, logs, layout);
      dirty = true;
      return;
    }
  }
}

void enable_quit_input_if_needed(int tty_fd, int input_fd, bool &input_enabled) {
  if (input_enabled) {
    return;
  }
  input_enabled = enable_input_mode(input_fd, g_runtime.saved_input_termios);
  g_runtime.input_mode_enabled = input_enabled;
  if (input_enabled) {
    write_all(tty_fd, "\033[?1000h\033[?1006h");
  }
}

void render_loop(int tty_fd, int input_fd, int control_pipe_read, int log_pipe_read, int uart_pipe_read,
                 const std::string program_name) {
  set_nonblocking(log_pipe_read);
  set_nonblocking(uart_pipe_read);
  set_nonblocking(control_pipe_read);

  PaneBuffer uart;
  PaneBuffer logs;
  AsyncFileWriter uart_log;
  AsyncFileWriter host_log;
  AsyncFileWriter all_log;
  ScreenSize screen = query_screen_size(tty_fd);
  SplitLayout layout = compute_layout(screen);
  ScrollState scroll;
  bool log_open = true;
  bool uart_open = true;
  bool finish_pending = false;
  bool finished = false;
  bool should_quit = false;
  bool dirty = true;
  bool exit_hint_written = false;
  std::chrono::steady_clock::time_point finish_ready_at;
  std::string pending_input;
  bool input_enabled = enable_input_mode(input_fd, g_runtime.saved_input_termios);
  g_runtime.input_mode_enabled = input_enabled;
  SplitLogPaths log_paths = make_split_log_paths();
  uart_log.start(log_paths.uart.c_str());
  host_log.start(log_paths.host.c_str());
  all_log.start(log_paths.all.c_str());

  write_all(tty_fd, input_enabled ? "\033[?1049h\033[?25l\033[?1007l\033[?1000h\033[?1006h"
                                  : "\033[?1049h\033[?25l\033[?1007l");

  while (!g_runtime.stopping && (!finished || (input_enabled && !should_quit))) {
    if (g_runtime.resize_requested.exchange(false)) {
      screen = query_screen_size(tty_fd);
      layout = compute_layout(screen);
      clamp_scroll_state(scroll, uart, logs, layout);
      dirty = true;
    }

    std::array<struct pollfd, 4> pfds{};
    pfds[0].fd = log_open ? log_pipe_read : -1;
    pfds[0].events = POLLIN;
    pfds[1].fd = uart_open ? uart_pipe_read : -1;
    pfds[1].events = POLLIN;
    pfds[2].fd = input_enabled ? input_fd : -1;
    pfds[2].events = POLLIN;
    pfds[3].fd = control_pipe_read;
    pfds[3].events = POLLIN;

    int rc = poll(pfds.data(), pfds.size(), 100);
    if (rc < 0 && errno != EINTR) {
      break;
    }
    if (g_runtime.stopping) {
      break;
    }
    if (g_runtime.finish_requested.exchange(false)) {
      finish_pending = true;
      finish_ready_at = std::chrono::steady_clock::now() + kFinishDrainQuietTime;
      dirty = true;
    }

    std::array<char, 512> buf{};
    bool saw_output = false;

    if (log_open) {
      log_open = drain_pipe_into_pane(pfds[0].fd, pfds[0].revents, logs, layout.right_width, scroll.log_offset,
                                      host_log, all_log, dirty, saw_output);
    }

    if (uart_open) {
      uart_open = drain_pipe_into_pane(pfds[1].fd, pfds[1].revents, uart, layout.left_width, scroll.uart_offset,
                                       uart_log, all_log, dirty, saw_output);
    }

    if (finish_pending && saw_output) {
      finish_ready_at = std::chrono::steady_clock::now() + kFinishDrainQuietTime;
    }

    const bool streams_finished = !log_open && !uart_open;
    if (streams_finished && !finished) {
      finish_pending = true;
      finish_ready_at = std::chrono::steady_clock::now();
      dirty = true;
    }

    if (finish_pending && !finished && std::chrono::steady_clock::now() >= finish_ready_at) {
      finished = true;
      finish_pending = false;
      g_runtime.finished = true;
      enable_quit_input_if_needed(tty_fd, input_fd, input_enabled);
    }

    if (finished) {
      append_exit_hint(logs, layout, scroll, dirty, exit_hint_written);
    }

    if (finished && !input_enabled) {
      break;
    }

    if (input_enabled && pfds[2].fd >= 0 && (pfds[2].revents & POLLIN)) {
      ssize_t n = read(input_fd, buf.data(), buf.size());
      if (n > 0) {
        std::vector<InputEvent> events;
        std::string uart_input;
        std::string_view input_chunk(buf.data(), static_cast<size_t>(n));
        consume_input(pending_input, input_chunk, finished, events, uart_input);
        if (!finished) {
          handle_uart_input(uart_input, uart, layout, scroll, dirty);
        }
        for (const InputEvent &event: events) {
          handle_input_event(event, scroll, uart, logs, layout, finish_pending, finish_ready_at, finished,
                             input_enabled, should_quit, dirty);
        }
      }
    }

    if (pfds[3].fd >= 0 && (pfds[3].revents & (POLLIN | POLLHUP))) {
      while (read(control_pipe_read, buf.data(), buf.size()) > 0) {}
      if (g_runtime.detach_requested.exchange(false)) {
        pending_input.clear();
        should_quit = true;
        detach_splitview_runtime(input_enabled);
      }
    }

    if (dirty) {
      if (g_runtime.stopping) {
        break;
      }
      clamp_scroll_state(scroll, uart, logs, layout);
      draw_screen(tty_fd, uart, logs, layout, scroll,
                  make_footer_text(program_name, finished, input_enabled, scroll, layout.cols));
      dirty = false;
    }
  }

  restore_input_mode(input_fd, g_runtime.saved_input_termios, input_enabled);
  g_runtime.input_mode_enabled = false;
  if (!g_runtime.input_capture_released) {
    finalize_terminal_cleanup(input_enabled);
  }
  all_log.finish();
  host_log.finish();
  uart_log.finish();
  close(log_pipe_read);
  close(uart_pipe_read);
}

} // namespace

void common_splitview_preinit(const char *program_name) {
  if (g_runtime.initialized.exchange(true)) {
    return;
  }

  if (!should_enable_splitview(program_name)) {
    return;
  }

  std::array<int, 2> log_pipe{};
  std::array<int, 2> uart_pipe{};
  std::array<int, 2> control_pipe{};

  if (pipe(log_pipe.data()) != 0 || pipe(uart_pipe.data()) != 0 || pipe(control_pipe.data()) != 0) {
    cleanup_pipe_array(log_pipe);
    cleanup_pipe_array(uart_pipe);
    cleanup_pipe_array(control_pipe);
    return;
  }

  g_runtime.saved_stdout = dup(STDOUT_FILENO);
  g_runtime.saved_stderr = dup(STDERR_FILENO);
  g_runtime.tty_fd = dup(STDOUT_FILENO);
  g_runtime.input_fd = open("/dev/tty", O_RDONLY);
  if (g_runtime.input_fd < 0 && isatty(STDIN_FILENO)) {
    g_runtime.input_fd = dup(STDIN_FILENO);
  }
  if (g_runtime.saved_stdout < 0 || g_runtime.saved_stderr < 0 || g_runtime.tty_fd < 0) {
    close_if_open(g_runtime.saved_stdout);
    close_if_open(g_runtime.saved_stderr);
    close_if_open(g_runtime.tty_fd);
    close_if_open(g_runtime.input_fd);
    cleanup_pipe_array(log_pipe);
    cleanup_pipe_array(uart_pipe);
    cleanup_pipe_array(control_pipe);
    return;
  }

  if (dup2(log_pipe[1], STDOUT_FILENO) < 0 || dup2(log_pipe[1], STDERR_FILENO) < 0) {
    close_if_open(g_runtime.saved_stdout);
    close_if_open(g_runtime.saved_stderr);
    close_if_open(g_runtime.tty_fd);
    close_if_open(g_runtime.input_fd);
    cleanup_pipe_array(log_pipe);
    cleanup_pipe_array(uart_pipe);
    cleanup_pipe_array(control_pipe);
    return;
  }

  close(log_pipe[1]);

  g_runtime.log_pipe_read = log_pipe[0];
  g_runtime.uart_pipe_read = uart_pipe[0];
  g_runtime.uart_pipe_write = uart_pipe[1];
  g_runtime.control_pipe_read = control_pipe[0];
  g_runtime.control_pipe_write = control_pipe[1];
  std::string runtime_program_name = program_name ? program_name : "difftest";

  struct sigaction sa_winch {};
  sa_winch.sa_handler = handle_sigwinch;
  sigemptyset(&sa_winch.sa_mask);
  sigaction(SIGWINCH, &sa_winch, nullptr);

  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);

  g_runtime.stopping = false;
  g_runtime.finish_requested = false;
  g_runtime.finished = false;
  g_runtime.detach_requested = false;
  g_runtime.input_capture_released = false;
  g_runtime.force_cleanup_started = false;
  g_runtime.active = true;
  maybe_start_external_uart_input();
  g_runtime.render_thread = std::thread(render_loop, g_runtime.tty_fd, g_runtime.input_fd, g_runtime.control_pipe_read,
                                        g_runtime.log_pipe_read, g_runtime.uart_pipe_read, runtime_program_name);
}

void common_splitview_finish() {
  if (!g_runtime.initialized || g_runtime.force_cleanup_started) {
    return;
  }
  if (!g_runtime.initialized.exchange(false)) {
    return;
  }

  fflush(stdout);
  fflush(stderr);

  restore_standard_streams();
  g_runtime.finish_requested = true;
  if (g_runtime.control_pipe_write >= 0) {
    static const char wake = 'f';
    write(g_runtime.control_pipe_write, &wake, 1);
  }

  if (g_runtime.render_thread.joinable()) {
    g_runtime.render_thread.join();
  }

  g_runtime.active = false;
  close_if_open(g_runtime.uart_pipe_write);
  close_if_open(g_runtime.tty_fd);
  close_if_open(g_runtime.input_fd);
  close_uart_input_fd();
  close_if_open(g_runtime.control_pipe_read);
  close_if_open(g_runtime.control_pipe_write);
  g_runtime.input_capture_released = false;
  g_runtime.finish_requested = false;
  g_runtime.finished = false;
  g_runtime.detach_requested = false;
  g_runtime.force_cleanup_started = false;
}

void common_splitview_force_cleanup() {
  if (!g_runtime.force_cleanup_started.exchange(true)) {
    g_runtime.stopping = true;
    g_runtime.active = false;

    restore_standard_streams();
    finalize_terminal_cleanup(false);

    close_if_open(g_runtime.uart_pipe_write);
    close_if_open(g_runtime.control_pipe_read);
    close_if_open(g_runtime.control_pipe_write);
    close_if_open(g_runtime.log_pipe_read);
    close_if_open(g_runtime.uart_pipe_read);
    close_if_open(g_runtime.input_fd);
    close_uart_input_fd();
    close_if_open(g_runtime.tty_fd);
  }
}

void common_splitview_request_finish() {
  if (!g_runtime.active || g_runtime.input_capture_released) {
    return;
  }
  if (g_runtime.finished) {
    common_splitview_request_detach();
    return;
  }

  g_runtime.finish_requested = true;
  if (g_runtime.control_pipe_write >= 0) {
    static const char wake = 'f';
    write(g_runtime.control_pipe_write, &wake, 1);
  }
}

void common_splitview_request_detach() {
  if (!g_runtime.active || g_runtime.input_capture_released || g_runtime.detach_requested.exchange(true)) {
    return;
  }

  if (g_runtime.control_pipe_write >= 0) {
    static const char wake = 'q';
    write(g_runtime.control_pipe_write, &wake, 1);
  }
}

void common_splitview_set_uart_input_fd(int fd) {
  const int old_fd = g_runtime.uart_input_fd.exchange(fd);
  if (old_fd >= 0) {
    close(old_fd);
  }
}

void common_splitview_set_log_path(const char *path) {
  if (!g_runtime.initialized && path != nullptr && path[0] != '\0') {
    g_splitview_log_path = path;
  }
}

bool common_splitview_is_active() {
  return g_runtime.active;
}

int common_splitview_uart_fd() {
  return g_runtime.active ? g_runtime.uart_pipe_write : -1;
}

#endif // CONFIG_SPLITVIEW
