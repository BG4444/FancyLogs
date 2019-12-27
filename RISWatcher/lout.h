#ifndef LOUT_H
#define LOUT_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <array>
#include <QString>
#include <QDateTime>

std::ostream& operator << (std::ostream& out,const std::string& in);

std::ostream& operator << (std::ostream& out,const QString& str);

class Lout
{
    static auto tm()
    {
        return std::chrono::system_clock::now();
    }
    constexpr static std::array<char,4> tickChars{'|','/','-','\\'};
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
    void nextTick();
public:
    size_t getWidth();
    const QLatin1String fmt;
    const size_t width;
    static constexpr size_t brWidth=6;
    template<typename T> std::ostream& operator << (const T& rhs)
    {
        std::cout  << '[';
        return std::operator <<(std::cout,QDateTime::currentDateTime().toString(fmt).toStdString()) << "]     " << rhs;
    }
    void anounse(const std::string& msg);
    void anounse(const QString& msg);
    void brackets(const std::string& str=std::string());
    void ok();
    void fail();
    void tick();
    void percent(const size_t cur,const size_t total);
    Lout();
};

extern Lout lout;

#endif // LOUT_H
