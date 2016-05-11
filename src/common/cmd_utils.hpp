
#pragma once

#include <string>

static
std::string get_app_name(char * app_exe)
{
    std::string app_name;
    std::size_t len = std::strlen(app_exe);
    char * end_ptr = app_exe;
    char * begin_ptr = app_exe + len;
    char * cur_ptr = begin_ptr;
    while (cur_ptr >= end_ptr) {
        if (*cur_ptr == '/' || *cur_ptr == '\\') {
            if (cur_ptr != begin_ptr) {
                break;
            }
        }
        cur_ptr--;
    }
    cur_ptr++;
    app_name = cur_ptr;
    return app_name;
}

static
int parse_number_u32(std::string::const_iterator & iterBegin,
    const std::string::const_iterator & iterEnd, unsigned int & num)
{
    int n = 0, digits = 0;
    std::string::const_iterator & iter = iterBegin;
    for (; iter != iterEnd; ++iter) {
        char ch = *iter;
        if (ch >= '0' && ch <= '9') {
            n = n * 10 + ch - '0';
            digits++;
        }
        else {
            break;
        }
    }
    if (digits > 0) {
        if (digits <= 10)
            num = n;
        else
            digits = 0;
    }
    return digits;
}

static
int parse_number_u32(const std::string & str, unsigned int & num)
{
    std::string::const_iterator iter = str.begin();
    return parse_number_u32(iter, str.end(), num);
}

static
bool is_valid_ip_v4(const std::string & ip)
{
    if (ip.empty() || ip.length() > 15)
        return false;

    unsigned int num;
    int digits = 0;
    int dots = 0;
    int num_cnt = 0;
    std::string::const_iterator iter;
    for (iter = ip.begin(); iter != ip.end(); ++iter) {
        digits = parse_number_u32(iter, ip.end(), num);
        if ((digits > 0) && (/*num >= 0 && */num < 256)) {
            num_cnt++;
            if (iter == ip.end())
                break;
            char ch = *iter;
            if (ch == '.')
                dots++;
            else
                break;
        }
        else
            return false;
    }
    return (dots == 3 && num_cnt == 4);
}

static
bool is_number_u32(const std::string & str)
{
    if (str.empty() || str.length() > 10)
        return false;

    std::string::const_iterator iter;
    for (iter = str.begin(); iter != str.end(); ++iter) {
        char ch = *iter;
        if (ch < '0' || ch > '9')
            return false;
    }
    return true;
}

static
bool is_socket_port(const std::string & port)
{
    unsigned int port_num = 0;
    int digits = parse_number_u32(port, port_num);
    return ((digits > 0) && (port_num > 0 && port_num < 65536));
}

static
bool get_cmd_value(const std::string & cmd, char sep, std::string & cmd_value)
{
    std::string::const_iterator iter;
    for (iter = cmd.begin(); iter != cmd.end(); ++iter) {
        char ch = *iter;
        if (ch == sep) {
            ++iter;
            cmd_value = cmd.substr(iter - cmd.begin());
            return true;
        }
    }
    return false;
}
