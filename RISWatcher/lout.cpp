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

std::ostream &operator <<(std::ostream &out, const QString &str)
{
    const auto txt = str.toUtf8().toStdString();
    return out << txt;
}

std::ostream &operator <<(std::ostream &out, const std::string &in)
{
    const size_t width=lout.getWidth();
    for(size_t i=0;;)
    {
        const size_t j=i;
        i+=width;
        if(i>=in.size())
        {
            out << std::left << std::setfill(' ') << std::setw(width+Lout::brWidth+2);
            std::operator<<(out, in.substr(j));
            out << flush;
            return out;
        }
        out << in.substr(j,i) << '\n' << std::right << std::setfill(' ') << std::setw(lout.fmt.size()+7)<<' ';
    }
}

void Lout::nextTick()
{
    if(++curTick==tickChars.cend())
    {
        curTick=tickChars.cbegin();
    }
}

void Lout::anounse(const std::string &msg)
{
    *this << std::left << msg << std::flush;
}

void Lout::anounse(const QString &msg)
{
    anounse(msg.toUtf8().toStdString());
}

void Lout::brackets(const string &str)
{

    const auto midpos=str.length()/2;
    const auto strHalfL=str.substr(0,midpos);
    const auto strHalfR=str.substr(midpos);

    constexpr size_t half=brWidth/2;

    cout << setfill('\b') << setw(brWidth+3) <<right<< '['<< setfill(' ')<<setw(half)<<right;
    std::operator<<(cout,strHalfL);
    cout<<setw(half)<<left;
    std::operator<<(cout,strHalfR);
    cout<<']'<<flush;

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

}
