#include "lout.h"

#include <iomanip>
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO

using namespace std;

Lout lout;

std::ostream &operator <<(std::ostream &out, const QString &str)
{
    return out << str.toUtf8().toStdString();
}

size_t getWidth()
{
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    return size.ws_col-25-8-6;
}

std::ostream &operator <<(std::ostream &out, const std::string &in)
{
    const size_t width=getWidth();
    for(size_t i=0;;)
    {
        const size_t j=i;
        i+=width;
        if(i>=in.size())
        {
            out << std::left << std::setfill(' ') << std::setw(width);
            std::operator<<(out, in.substr(j));
            return out;
        }
        out << in.substr(j,i) << '\n' << std::left << std::setfill(' ') << std::setw(29);
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
    cout<<'['<<setw(half)<<right;
    std::operator<<(cout,strHalfL);
    cout<<setw(half)<<left;
    std::operator<<(cout,strHalfR);
    cout<<']';

}

void Lout::ok()
{
    brackets("OK");
}

void Lout::fail()
{
    brackets("FAIL");
}

void Lout::tick()
{
    cout << setfill('\b') << setw(brWidth+1) << '\b';
}
