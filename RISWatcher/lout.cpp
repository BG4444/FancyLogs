#include "lout.h"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
    size_t Lout::getWidth()
    {
        CONSOLE_SCREEN_BUFFER_INFO nfo;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&nfo);
        return nfo.srWindow.Right-nfo.srWindow.Left-width;
    }
#else
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
    size_t Lout::getWidth()
    {
        struct winsize size;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
        return size.ws_col-width;
    }
#endif

using namespace std;

Lout lout;

constexpr std::array<char,4> Lout::tickChars;

Lout& operator <<(Lout &out, const QString &str)
{
    const auto txt = str.toUtf8().toStdString();
    return out << txt;
}

Lout& operator <<(Lout &out, const std::string &in)
{
    if(!out.canMessage())
    {
        return out;
    }
    const size_t width=out.getWidth();  //width of text area
    for(size_t i=0;;)
    {
        const size_t j=i;
        const auto oldX = out.getLastX();
        const auto add=width - oldX;
        const auto len = in.size();
        i+= add;

        if(i>=len)
        {
            const auto outS=in.substr(j);
            out.shift(outS.size());
            cout << outS;
            return out << flush;
        }

        cout << in.substr(j,i-j);
        out.newLine();
    }    
}

void Lout::nextTick()
{
    if(canMessage())
    {
        if(++curTick==tickChars.cend())
        {
            curTick=tickChars.cbegin();
        }
        resetX();
        shift(getWidth());
        cout << setw(brWidth+2) << setfill('\b') << '\b';
    }
}

void Lout::shift(size_t count)
{
    lastX+=count;
}

void Lout::resetX()
{
    lastX=0;
}

void Lout::newLine()
{
    if(canMessage())
    {
        lout.resetX();
        cout << '\n' << std::right << std::setfill(' ') << std::setw(fmt.size()+7)<<' ';
    }
}

size_t Lout::getLastX() const
{
    return lastX;
}

void Lout::brackets(const string &str)
{
    if(canMessage())
    {
        const auto midpos=str.length()/2;
        const auto strHalfL=str.substr(0,midpos);
        const auto strHalfR=str.substr(midpos);

        constexpr size_t half=brWidth/2;

        if(getWidth() - lastX)
        {
            cout << std::left << std::setfill(' ') << std::setw(getWidth() - lastX) <<  ' ';
        }
        cout << right<< '['<< setfill(' ')<<setw(half)<<right;
        std::operator<<(cout,strHalfL);
        cout<<setw(half)<<left;
        std::operator<<(cout,strHalfR);
        cout<<']'<<flush;

        resetX();
    }
}

void Lout::ok()
{
    brackets("OK");
    cout << '\n';
}

void Lout::fail()
{
    brackets("FAIL");
    cout << '\n';
}

void Lout::tick()
{    
    brackets(std::string(curTick,1));
    nextTick();
}

void Lout::percent(const size_t cur, const size_t total)
{

    const size_t percent = 100 * cur / total;
    stringstream str;
    str<< *curTick<<' '<<setw(3)<<percent<<'%';
    brackets(str.str());
    nextTick();
}

Lout::Lout():fmt("dd.MM.yyyy hh:mm:ss.zzz"),width(fmt.size()+1+brWidth+8)
{
    *this <<"width is " << getWidth() << "\n";
}

bool Lout::canMessage() const
{
    return msgLevel <= outLevel;
}

void Lout::setMsgLevel(const Lout::LogLevel lvl)
{
    msgLevel=lvl;
}

Lout &operator <<(Lout &out, const Lout::LogLevel lvl)
{
    out.setMsgLevel(lvl);
    return out;
}

Lout &operator <<(Lout &out, const char *rhs)
{
    return out<<string(rhs);
}


Lout &operator <<(Lout &out, const size_t rhs)
{
    return out << to_string(rhs);
}

Lout& anounce(Lout &ret)
{
    if(ret.canMessage())
    {
        cout << '['
             << QDateTime::currentDateTime().toString(ret.fmt).toStdString()
             << "]     ";
    }
    return ret;
}

Lout &operator <<(Lout &out, Lout &(*func)(Lout &))
{
    return func(out);
}

Lout &operator <<(Lout &out, const char rhs)
{
    if(out.canMessage())
    {
        cout << rhs;
    }
    return out;
}

Lout &endl(Lout &out)
{
    if(out.canMessage())
    {
        cout << endl;
        out.resetX();
    }
    return out;
}

Lout &flush(Lout &out)
{
    if(out.canMessage())
    {
        cout << flush;
    }
    return out;
}
