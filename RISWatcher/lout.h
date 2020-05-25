#ifndef LOUT_H
#define LOUT_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <array>
#include <stack>
#include <QString>
#include <QDateTime>
#include <functional>


class Lout
{
public:
    enum LogLevel
    {
        Info,
        WorkFlow,
        Debug,
        Trace,
        DeepTrace
    };
private:
    std::stack<LogLevel> logLevels;
    static auto tm();
    constexpr static std::array<char,4> tickChars{'|','/','-','\\'};
    std::array<char,4>::const_iterator curTick=tickChars.cbegin();
    void nextTick();        
    std::stack<size_t> lastX;
    LogLevel msgLevel=Info;
    LogLevel outLevel=Info;
    bool lastWasBrackets = true;
    bool hasAnounce = false;
    void indent(const size_t cnt, const char inner, const char chr);
    void indentLineStart();
    void noBr();
    void preIndent();
    void printBrackets(const std::string &str, const int color);

public:
    void newLine();
    void shift(size_t count);
    void resetX();
    size_t getLastX() const;
    size_t getWidth();
    const QLatin1String fmt;
    const size_t width;
    static constexpr size_t brWidth=6;        
    Lout &brackets(const std::string& str, const int color);
    void tick();
    void percent(const size_t cur,const size_t total);
    Lout();    
    bool canMessage() const;
    void pushMsgLevel(const LogLevel lvl);
    void popMsgLevel();
    void setOutLevel(const LogLevel outLevel);    
    void doAnounce();
    void print(const std::string& in);
};

Lout& operator << (Lout& out, const Lout::LogLevel lvl);
Lout& operator << (Lout& out, const std::string& in);
Lout& operator << (Lout& out, const QString& str);
Lout& operator << (Lout& out, Lout& (*func)(Lout&));
Lout& operator << (Lout& out, std::function<Lout& (Lout&)>&&func);
Lout& operator << (Lout& out, const char* rhs);
Lout& operator << (Lout& out, const size_t rhs);
Lout& operator << (Lout& out, const char rhs);
Lout& operator << (Lout& out, const int rhs);

Lout &anounce(Lout &ret);
Lout &endl(Lout &ret);
Lout &flush(Lout& out);
Lout &ok(Lout& out);
Lout &fail(Lout& out);
Lout &newLine(Lout& out);
Lout &pop(Lout& out);
Lout &Color(Lout& out, const int);
Lout &noColor(Lout& out);

extern Lout lout;

#endif // LOUT_H
