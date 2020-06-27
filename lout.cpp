#include "lout.h"
#include <sstream>
#include <iomanip>
#include <QObject>
#include <unicode/utf8.h>

using namespace std;

#ifdef _WIN32
#include <windows.h>

    size_t Lout::getWidth()
    {
        CONSOLE_SCREEN_BUFFER_INFO nfo;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&nfo);
        return nfo.srWindow.Right-nfo.srWindow.Left-width;
    }

    Lout &Color(Lout &out, const int color)
    {
        return out;
    }

    Lout &noColor(Lout &out)
    {
        return out;
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

    Lout &Color(Lout &out, const int color)
    {
        if(out.canMessage())
        {
            cout << "\033[1;" << color << 'm';
        }
        return out;
    }

    Lout &noColor(Lout &out)
    {
        if(out.canMessage())
        {
            cout << "\033[0m";
        }
        return out;
    }

#endif

Lout lout;

constexpr std::array<char,4> Lout::tickChars;

Lout& operator <<(Lout &out, const QString &str)
{    
    const auto txt = str.toUtf8().toStdString();
    return out << txt;
}

Lout& operator <<(Lout &out, const std::string &in)
{
    out.print(in);
    return out;
}

auto Lout::tm()
{
    return std::chrono::system_clock::now();
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
        indent(brWidth+1,'\b','\b');
        noBr();
    }
}

void Lout::shift(size_t count)
{
    lastX.top()+=count;
}

void Lout::resetX()
{
    lastX.top()=0;
}

size_t Lout::getLastX() const
{
    return lastX.top();
}

void Lout::printBrackets(const string& str, const int color)
{
    const auto midpos=str.length()/2;
    const auto strHalfL=str.substr(0,midpos);
    const auto strHalfR=str.substr(midpos);
    constexpr size_t half=brWidth/2;
    *this << bind(Color, placeholders::_1, color);
    cout << right << '['<< setfill(' ') << setw(half) << right;
    std::operator<<(cout,strHalfL);
    cout << setw(half) << left;
    std::operator<<(cout,strHalfR);
    cout << ']';
    *this<<noColor << flush;
}

Lout& Lout::brackets(const string &str, const int color )
{
    if(canMessage())
    {
        if(lastWasBrackets)
        {
            cout << '\n';
        }
        else
        {
            const auto countOfindention = getWidth() - lastX.top();
            indent(countOfindention, ' ', ' ');
            printBrackets(str, color);
        }

        if(lastX.size()>1)
        {
            lastX.pop();

            indent((lastX.size()-1)*4, ' ', ' ');
            indent(4, '_', '/');

            shift(-lastX.size()*4);

            if(lastWasBrackets)
            {
                const auto countOfindention = getWidth()  + fmt.size() + 7 - lastX.size() * 4;
                indent(countOfindention, ' ', ' ');
                printBrackets(str, color);
            }
        }
        else
        {
            resetX();
        }
        lastWasBrackets = true;
        hasAnounce = false;
    }
    return *this;
}

void Lout::tick()
{    
    brackets(std::string(curTick,1), 33);
    nextTick();
}

void Lout::percent(const size_t cur, const size_t total)
{
    if(canMessage())
    {
        const size_t percent = 100 * cur / total;
        stringstream str;
        str << *curTick << ' ' << setw(3) << percent << '%';
        brackets(str.str(),33);
        nextTick();
    }
}

Lout::Lout():fmt("dd.MM.yyyy hh:mm:ss.zzz"),width(fmt.size()+1+brWidth+8)
{
    lastX.push(0);
    *this << Trace << "width is " << getWidth() << '\n' << pop;
}

bool Lout::canMessage() const
{
    return msgLevel <= outLevel;
}

void Lout::popMsgLevel()
{
    if(logLevels.empty())
    {
        pushMsgLevel(Info);
        *this << endl
              << anounce
              << QObject::tr("Wrong message stack balance!")
              << fail;
        exit(-1);
    }
    msgLevel=logLevels.top();
    logLevels.pop();
}

void Lout::setOutLevel(const Lout::LogLevel outLevel)
{
    this->outLevel=outLevel;
}

void Lout::noBr()
{
    lastWasBrackets = false;
    hasAnounce = false;
}

void Lout::indent(const size_t cnt, const char inner, const char chr)
{
    if(cnt)
    {
        cout << std::right << std::setfill(inner) << std::setw(cnt) <<  chr;
    }
}

void Lout::flood(size_t cnt, const string& chr)
{
    while(cnt--)
    {
        *this << chr;
    }
}

void Lout::indentLineStart()
{
   indent(fmt.size()+7+getLastX(),' ', ' ');
}

void Lout::newLine()
{
    if(canMessage())
    {
        resetX();
        cout << '\n';
        indentLineStart();
        hasAnounce = true;
        lastWasBrackets = false;
    }
}

void Lout::doAnounce()
{
    if(canMessage())
    {
        cout << '\n';
        if(lastWasBrackets && !getLastX())
        {

        }
        else
        {
            const auto old = lastX.size();
            const auto cnt = old*4;
            lastX.push(cnt);

            indent((old-1)*4, ' ', ' ');

            cout << '\\';
            indent(3, '_', ' ');

            cout << '\n';
            indent(cnt, ' ', '\\');

            cout << '\n';
            indent(cnt,' ', ' ');
        }


        cout << '['
             <<  QDateTime::currentDateTime().toString(fmt).toStdString()
             << "]\033[0m     ";
        lastWasBrackets = false;
        hasAnounce = true;
    }
}

void Lout::preIndent()
{
    if(!hasAnounce && lastWasBrackets)
    {
        indentLineStart();
    }
}

size_t strlen(const std::string& in)
{
    size_t ret=0;
    const int len = in.length();
    for(int i=0;i < len;++ret)
    {
        UChar32 c;
        U8_NEXT(in.c_str(), i, len, c);
    }
    return ret;
}

int roll(const std::string& in, int i, size_t pos)
{
    const int len=in.length();
    while(pos--)
    {
        UChar32 c;
        U8_NEXT(in.c_str(), i, len, c);
    }
    return i;
}

string substr(const std::string& in,const size_t pos)
{
    return in.substr(roll(in, 0, pos));
}

string substr(const std::string& in, const size_t pos, const size_t count)
{
    const int beg = roll(in, 0,   pos);
    const int  en = roll(in, beg, count);
    return in.substr(beg, en);
}

void Lout::print(const string &in)
{
    if(canMessage())
    {
        const size_t width=getWidth();  //width of text area

        preIndent();

        noBr();

        for(size_t i=0;;)
        {
            const size_t j=i;
            const auto oldX = getLastX();
            const auto add=width - oldX;
            const auto len = strlen(in);
            i+= add;

            if(i>=len)
            {
                const auto& outS=substr(in, j);
                shift(strlen(outS));
                cout << outS;
                *this << flush;
                return;
            }
            cout << substr(in, j, i-j) << '\n';
            resetX();
            indentLineStart();
        }
    }
}

void Lout::pushMsgLevel(const Lout::LogLevel lvl)
{
    logLevels.push(msgLevel);
    msgLevel=lvl;    
}

Lout &operator <<(Lout &out, const Lout::LogLevel lvl)
{    
    out.pushMsgLevel(lvl);
    return out;
}

Lout &operator <<(Lout &out, const char *rhs)
{
    return out << string(rhs);
}


Lout &operator <<(Lout &out, const size_t rhs)
{
    return out << to_string(rhs);
}

Lout& anounce(Lout &ret)
{
    ret.doAnounce();
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
        cout << string(1, rhs);
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

Lout &ok(Lout &out)
{    
    out.brackets("OK",32);
    return out;
}

Lout &fail(Lout &out)
{
    out.brackets("FAIL",31);
    return out;
}

Lout &newLine(Lout &out)
{    
    out.newLine();
    return out;
}

Lout &pop(Lout &out)
{
    lout.popMsgLevel();
    return out;
}

Lout &operator <<(Lout &out, const int rhs)
{
     return out << to_string(rhs);
}

Lout &operator <<(Lout &out, std::function<Lout &(Lout &)> &&func)
{
    return func(out);
}

Lout &operator <<(Lout &out, const time_t rhs)
{
    return out << to_string(rhs);
}

void Lout::printHist(const map<uint8_t, size_t> &in)
{
    const size_t maxV = in.empty() ? 0 : std::max_element(in.cbegin(), in.cend())->second;
    const ssize_t M = maxV > 20 ? 20 : maxV;

    static string bars[]={"\u2591", "\u2588"};

    constexpr size_t captionWidth = 8;
    const auto width = getWidth() - captionWidth;
    const auto len = min(in.size(), width);
    const auto start = (width - len) / 2;

    for(ssize_t i=0;  i < (20 - M) * ssize_t(width); ++i)
    {
        lout << bars[0];
    }
    if(M)
    {
        const auto m = maxV / M;
        for(size_t i=M; i--; )
        {
            const auto value = i*m;

            string caption = to_string(value);
            if(caption.size() > captionWidth-1)
            {
                caption.resize(captionWidth-1);
            }
            caption.append(captionWidth-caption.size(), ' ');

            flood(start, bars[0]);
            auto pos = in.cbegin();
            for(size_t j = 0; j<len;++j,++pos)
            {
                lout << bars[pos->second >= value];

            }
            flood(width-len-start, bars[0]);
        }
    }
}
