/* Author: baichen.bai@alibaba-inc.com */


#include "deg/logger.h"


Printer::Printer(std::ostream& stream, const std::string& format):
    stream(stream),
    format(format.c_str()),
    ptr(format.c_str()),
    cont(false) {
    property.flags = stream.flags();
    property.fill = stream.fill();
    property.precision = stream.precision();
    property.width = stream.width();
}


Printer::Printer(std::ostream& stream, const char* format):
    stream(stream),
    format(format),
    ptr(format),
    cont(false) {
    property.flags = stream.flags();
    property.fill = stream.fill();
    property.precision = stream.precision();
    property.width = stream.width();
}


void Printer::process() {
    formatter.clear();

    size_t len;

    while (*ptr) {
        switch (*ptr) {
            case '%':
                if (ptr[1] != '%') {
                    process_flag();
                    return;
                }
                stream.put('%');
                ptr += 2;
                break;
            case '\n':
                stream << std::endl;
                ++ptr;
                break;
            case '\r':
                ++ptr;
                if (*ptr != '\n')
                    stream << std::endl;
                break;
            default:
                len = strcspn(ptr, "%\n\r\0");
                stream.write(ptr, len);
                ptr += len;
                break;
        }
    }
}


void Printer::process_flag() {
    bool done = false;
    bool end_number = false;
    bool have_precision = false;
    int number = 0;

    stream.fill(' ');
    stream.flags((std::ios::fmtflags)0);

    while (!done) {
        ++ptr;
        if (*ptr >= '0' && *ptr <= '9') {
            if (end_number)
                continue;
        } else if (number > 0) {
            end_number = true;
        }

        switch (*ptr) {
            case 's':
                formatter.format = Formatter::STRING;
                done = true;
                break;
            case 'c':
                formatter.format = Formatter::CHARACTER;
                done = true;
                break;
            case 'l':
                continue;
            case 'p':
                formatter.format = Formatter::INTEGER;
                formatter.base = Formatter::HEX;
                formatter.alternate_form = true;
                done = true;
                break;
            case 'X':
                formatter.uppercase = true;
                [[fallthrough]];
            case 'x':
                formatter.base = Formatter::HEX;
                formatter.format = Formatter::INTEGER;
                done = true;
                break;
            case 'o':
                formatter.base = Formatter::OCT;
                formatter.format = Formatter::INTEGER;
                done = true;
                break;
            case 'd':
            case 'i':
            case 'u':
                formatter.format = Formatter::INTEGER;
                done = true;
                break;
            case 'G':
                formatter.uppercase = true;
                [[fallthrough]];
            case 'g':
                formatter.format = Formatter::FLOAT;
                formatter.float_format = Formatter::BEST;
                done = true;
                break;
            case 'E':
                formatter.uppercase = true;
                [[fallthrough]];
            case 'e':
                formatter.format = Formatter::FLOAT;
                formatter.float_format = Formatter::SCIENTIFIC;
                done = true;
                break;
            case 'f':
                formatter.format = Formatter::FLOAT;
                formatter.float_format = Formatter::FIXED;
                done = true;
                break;
            case 'n':
                stream << "we do not do %n!!!\n";
                done = true;
                break;
            case '#':
                formatter.alternate_form = true;
                break;
            case '-':
                formatter.flush_left = true;
                break;
            case '+':
                formatter.print_sign = true;
                break;
            case ' ':
                formatter.blank_space = true;
                break;
            case '.':
                formatter.width = number;
                formatter.precision = 0;
                have_precision = true;
                number = 0;
                end_number = false;
                break;
            case '0':
                if (number == 0) {
                    formatter.fill_zero = true;
                    break;
                }
                [[fallthrough]];
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                number = number * 10 + (*ptr - '0');
                break;
            case '*':
                if (have_precision)
                    formatter.get_precision = true;
                else
                    formatter.get_width = true;
                break;
            case '%':
                DEG_UNREACHABLE;
                break;
            default:
                done = true;
                break;
        }

        if (end_number) {
            if (have_precision)
                formatter.precision = number;
            else
                formatter.width = number;
            end_number = false;
            number = 0;
        }

        if (done) {
            if ((formatter.format == Formatter::INTEGER) && have_precision) {
                // specified a . but not a float, set width
                formatter.width = formatter.precision;
                // precision requries digits for width, must fill with 0
                formatter.fill_zero = true;
            } else if (
                (formatter.format == Formatter::FLOAT) && !have_precision && \
                formatter.fill_zero
            ) {
                // ambiguous case, matching printf
                formatter.precision = formatter.width;
            }
        }
    } // end while

    ++ptr;
}


void Printer::end_args() {
    size_t len;

    while (*ptr) {
        switch (*ptr) {
            case '%':
                if (ptr[1] != '%')
                    stream << "<extra arg>";

                stream.put('%');
                ptr += 2;
                break;
            case '\n':
                stream << std::endl;
                ++ptr;
                break;
            case '\r':
                ++ptr;
                if (*ptr != '\n')
                    stream << std::endl;
                break;
            default:
                len = strcspn(ptr, "%\n\r\0");
                stream.write(ptr, len);
                ptr += len;
                break;
        }
    }

    stream.flags(property.flags);
    stream.fill(property.fill);
    stream.precision(property.precision);
    stream.width(property.width);
}


void Logger::log_printf(Printer& printer) {
    printer.end_args();
}


void Logger::_log(const std::string& name, const std::string& message) {
    if (!name.empty())
        stream << '[' << name << ']' << ": ";
    stream << message;
    stream.flush();
}


Logger* logger = nullptr;


Logger* get_logger() {
    if (!logger) {
        logger = new Logger(std::cout);
    }
    return logger;
}
