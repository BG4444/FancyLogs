#ifndef LOUT_H
#define LOUT_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <array>
#include <QString>
#include <QDateTime>

class Lout
{
public:
    enum LogLevel
    {
        Info,
        Debug
    };
private:
    static auto tm()
    {
        return std::chrono::system_clock::now();
    }
    constexpr static std::array<char,4> tickChars{'|','/','-','\\'};
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
    void nextTick();        
    size_t lastX=0;
    LogLevel msgLevel=Info;
    LogLevel outLevel=Info;
public:   
    void shift(size_t count);
    void resetX();
    size_t getLastX() const;
    size_t getWidth();
    const QLatin1String fmt;
    const size_t width;
    static constexpr size_t brWidth=6;        
    Lout &brackets(const std::string& str=std::string());
    void tick();
    void percent(const size_t cur,const size_t total);
    Lout();    
    bool canMessage() const;
    void setMsgLevel(const LogLevel lvl);
};

Lout& operator << (Lout& out, const Lout::LogLevel lvl);
Lout& operator << (Lout& out, const std::string& in);
Lout& operator << (Lout& out, const QString& str);
//Lout& operator << (Lout& out, std::ostream& (*func)(std::ostream&));
Lout& operator << (Lout& out, Lout& (*func)(Lout&));
Lout& operator << (Lout& out, const char* rhs);
Lout& operator << (Lout& out, const size_t rhs);
Lout& operator << (Lout& out, const char rhs);

Lout &anounce(Lout &ret);
Lout &endl(Lout &ret);
Lout &flush(Lout& out);
Lout &ok(Lout& out);
Lout &fail(Lout& out);
Lout &newLine(Lout& out);

extern Lout lout;

#endif // LOUT_H
