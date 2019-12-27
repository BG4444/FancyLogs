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
    const std::array<char,4> tickChars = {'|','/','-','\\'};
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
    void nextTick();
public:
    static constexpr size_t brWidth=6;
    template<typename T> std::ostream& operator << (const T& rhs)
    {
        return std::cout << '['<< QDateTime::currentDateTime().toString() << "]     " << rhs;
    }
    void anounse(const std::string& msg);
    void anounse(const QString& msg);
    void brackets(const std::string& str=std::string());
    void ok();
    void fail();
    void tick();
    void percent(const size_t cur,const size_t total);
};

extern Lout lout;

#endif // LOUT_H
