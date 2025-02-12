/* Author: baichen.bai@alibaba-inc.com */


#ifndef __LOGGER_H_
#define __LOGGER_H_


#include <ios>
#include <cstdio>
#include <string>
#include <ostream>
#include <sstream>
#include <iostream>
#include "utils.h"


struct Formatter {
    enum {
        DEC,
        HEX,
        OCT
    } base;

    enum {
        NONE,
        STRING,
        INTEGER,
        CHARACTER,
        FLOAT
    } format;

    enum {
        BEST,
        FIXED,
        SCIENTIFIC
    } float_format;

    bool alternate_form;
    bool flush_left;
    bool print_sign;
    bool blank_space;
    bool fill_zero;
    bool uppercase;

    int precision;
    int width;
    bool get_precision;
    bool get_width;

    Formatter() { clear(); }

    void clear() {
        alternate_form = false;
        flush_left = false;
        print_sign = false;
        blank_space = false;
        fill_zero = false;
        uppercase = false;
        base = DEC;
        format = NONE;
        float_format = BEST;
        precision = -1;
        width = 0;
        get_precision = false;
        get_width = false;
    }
};


class Printer {
public:
    Printer(std::ostream&, const std::string&);
    Printer(std::ostream&, const char*);
    ~Printer() = default;

public:
    int get_number(int val) {
        return val;
    }

    template <typename T>
    int get_number(const T& val) {
        return 0;
    }

    template <typename T>
    void add_arg(const T& val) {
        if (!cont)
            process();
        if (formatter.get_width) {
            formatter.get_width = false;
            cont = true;
            formatter.width = get_number(val);
            return;
        }
        if (formatter.get_precision) {
            formatter.get_precision = false;
            cont = true;
            formatter.precision = get_number(val);
            return;
        }

        switch (formatter.format) {
            case Formatter::CHARACTER:
                format_char(stream, val, formatter);
                break;
            case Formatter::INTEGER:
                format_integer(stream, val, formatter);
                break;
            case Formatter::FLOAT:
                format_float(stream, val, formatter);
                break;
            case Formatter::STRING:
                format_string(stream, val, formatter);
                break;
            default:
                stream << "<bad format>";
                break;
        }
    }

    void end_args();

protected:
    void process();
    void process_flag();

protected:
    std::ostream& stream;
    const char* format;
    const char* ptr;
    bool cont;
    struct Property {
        std::ios::fmtflags flags;
        char fill;
        int precision;
        int width;
    };
    struct Property property;
    Formatter formatter;
};


class Logger {
public:
    Logger() = delete;
    Logger(std::ostream& stream): stream(stream) {};
    ~Logger() = default;

protected:
    void log_printf(Printer&);

    template<typename T, typename... Args>
    void log_printf(Printer& printer, const T& val, const Args&... args) {
        printer.add_arg(val);
        log_printf(printer, args...);
    }

    template<typename... Args>
    void log_printf(std::ostream& stream, const char* fmt, const Args&... args) {
        Printer printer(stream, fmt);
        log_printf(printer, args...);
    }

public:
    std::ostream& get_tream() { return stream; }

    /* log a single message */
    template <typename... Args>
    void log(const std::string& name, const char* fmt, const Args&... args) {
        if (name.empty())
            return;
        std::ostringstream msg;
        log_printf(msg, fmt, args...);
        _log(name, msg.str());
    }

    void _log(const std::string&, const std::string&);

protected:
    std::ostream& stream;
};


/* format character */


template <typename T>
static inline void
_format_char(std::ostream& out, const T& data, Formatter& fmt) {
    out << data;
}


template <typename T>
static inline void
format_char(std::ostream& out, const T& data, Formatter& fmt) {
    out << "<bad arg type for char format>";
}


static inline void
format_char(std::ostream& out, char data, Formatter& fmt) {
    _format_char(out, data, fmt);
}


static inline void
format_char(std::ostream& out, unsigned char data, Formatter& fmt) {
    _format_char(out, data, fmt);
}


static inline void
format_char(std::ostream& out, signed char data, Formatter& fmt) {
    _format_char(out, data, fmt);
}


static inline void
format_char(std::ostream& out, short data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, unsigned short data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, int data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, unsigned int data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, long data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, unsigned long data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, long long data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


static inline void
format_char(std::ostream& out, unsigned long long data, Formatter& fmt) {
    _format_char(out, (char)data, fmt);
}


/* format integer */


template <typename T>
static inline void
_format_integer(std::ostream& out, const T& data, Formatter& fmt) {
    std::ios::fmtflags flags(out.flags());

    switch (fmt.base) {
        case Formatter::HEX:
            out.setf(std::ios::hex, std::ios::basefield);
            break;
        case Formatter::OCT:
            out.setf(std::ios::oct, std::ios::basefield);
            break;
        case Formatter::DEC:
            out.setf(std::ios::dec, std::ios::basefield);
            break;
    }

    if (fmt.alternate_form) {
        if (!fmt.fill_zero) {
            out.setf(std::ios::showbase);
        } else {
            switch (fmt.base) {
                case Formatter::HEX:
                    out << "0x";
                    fmt.width -= 2;
                    break;
                case Formatter::OCT:
                    out << "0";
                    fmt.width -= 1;
                    break;
                case Formatter::DEC:
                    break;
            }
        }
    }

    if (fmt.fill_zero)
        out.fill('0');
    if (fmt.width > 0)
        out.width(fmt.width);
    if (fmt.flush_left && !fmt.fill_zero)
        out.setf(std::ios::left);
    if (fmt.print_sign)
        out.setf(std::ios::showpos);
    if (fmt.uppercase)
        out.setf(std::ios::uppercase);

    out << data;
    out.flags(flags);
}


template <typename T>
static inline void
format_integer(std::ostream& out, const T& data, Formatter& fmt) {
    _format_integer(out, data, fmt);
}


static inline void
format_integer(std::ostream& out, char data, Formatter& fmt) {
    _format_integer(out, (int)data, fmt);
}


static inline void
format_integer(std::ostream& out, unsigned char data, Formatter& fmt) {
    _format_integer(out, (int)data, fmt);
}


static inline void
format_integer(std::ostream& out, signed char data, Formatter& fmt) {
    _format_integer(out, (int)data, fmt);
}


static inline void
format_integer(std::ostream& out, const unsigned char* data, Formatter& fmt) {
    _format_integer(out, (std::size_t)data, fmt);
}


static inline void
format_integer(std::ostream& out, const signed char* data, Formatter& fmt) {
    _format_integer(out, (std::size_t)data, fmt);
}


/* format float */


template <typename T>
static inline void
_format_float(std::ostream& out, const T& data, Formatter& fmt) {
    std::ios::fmtflags flags(out.flags());

    if (fmt.fill_zero)
        out.fill('0');

    switch (fmt.float_format) {
        case Formatter::SCIENTIFIC:
            if (fmt.precision != -1) {
                if (fmt.width > 0)
                    out.width(fmt.width);
                if (fmt.precision == 0)
                    fmt.precision = 1;
                else
                    out.setf(std::ios::scientific);
                out.precision(fmt.precision);
            } else if (fmt.width > 0) {
                out.width(fmt.width);
            }
            if (fmt.uppercase)
                out.setf(std::ios::uppercase);
            break;
        case Formatter::FIXED:
            if (fmt.precision != -1) {
                if (fmt.width > 0)
                    out.width(fmt.width);
                out.setf(std::ios::fixed);
                out.precision(fmt.precision);
            } else if (fmt.width > 0) {
                out.width(fmt.width);
            }
            break;
        default:
            if (fmt.precision != -1)
                out.precision(fmt.precision);
            if (fmt.width > 0)
                out.width(fmt.width);
            break;
    }

    out << data;
    out.flags(flags);
}


template <typename T>
static inline void
format_float(std::ostream& out, const T& data, Formatter& fmt) {
    out << "<bad arg type for float format>";
}


static inline void
format_float(std::ostream& out, float data, Formatter& fmt) {
    _format_float(out, data, fmt);
}


static inline void
format_float(std::ostream& out, double data, Formatter& fmt) {
    _format_float(out, data, fmt);
}


/* format string */


template <typename T>
static inline void
_format_string(std::ostream& out, const T& data, Formatter& fmt) {
    if (fmt.width > 0) {
        std::stringstream foo;
        foo << data;
        int flen = foo.str().size();

        if (fmt.width > flen) {
            char spaces[fmt.width - flen + 1];
            std::memset(spaces, ' ', fmt.width - flen);
            spaces[fmt.width - flen] = 0;

            if (fmt.flush_left)
                out << foo.str() << spaces;
            else
                out << spaces << foo.str();
        } else {
            out << data;
        }
    } else {
        out << data;
    }
}


template <typename T>
static inline void
format_string(std::ostream& out, const T& data, Formatter& fmt) {
    _format_string(out, data, fmt);
}


Logger* get_logger();


#endif
