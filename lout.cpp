#include "lout.h"
#include <iomanip>
#include <QObject>
#include <unicode/utf8.h>
#include <iomanip>
#include <cmath>
#include <cassert>

using namespace std;

#ifdef _WIN32
namespace win {
#include <windows.h>

}
    size_t Lout::getWidth()
    {
        using namespace  win;
        if(reinterpret_cast<void*>(&output)==reinterpret_cast<void*>(&cout))
        {
            CONSOLE_SCREEN_BUFFER_INFO nfo;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&nfo);
            return nfo.srWindow.Right-nfo.srWindow.Left-width;
        }
        else
        {
            return 60;
        }
    }

    Lout &Color(Lout &out, const uint8_t )
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
        if(output.str.get()==&cout)
        {
            winsize size={0};
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
            return size.ws_col-width;
        }
        return 60;
    }

    Lout &Color(Lout &out, const uint8_t color)
    {
        if(out.canMessage())
        {
            lock_guard lck(out.output.mtx);
            *out.output.str << "\033[1;" << int(color) << 'm';
        }
        return out;
    }

    Lout &noColor(Lout &out)
    {
        if(out.canMessage())
        {
            lock_guard lck(out.output.mtx);
            *out.output.str << "\033[0m";
        }
        return out;
    }

#endif

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
    lock_guard lck(output.mtx);
    const auto midpos=str.length()/2;
    const auto strHalfL=str.substr(0,midpos);
    const auto strHalfR=str.substr(midpos);
    constexpr size_t half=brWidth/2;
    *this << setColor(color);
    *output.str << right << '['<< setfill(' ') << setw(half) << right;
    std::operator<<(*output.str,strHalfL);
    *output.str << setw(half) << left;
    std::operator<<(*output.str,strHalfR);
    *output.str << ']';
    *this<<noColor << flush;
}

Lout& Lout::brackets(const string &str, const int color )
{
    if(canMessage())
    {
        string threadLogs;
        {
            unique_lock lck(globalMtx);
            for(auto&i:storedLogs)
            {
                lock_guard lck(i.mtx);
                if(i.str.get()!=&cout && i.lastWasBrackets)
                {
                    const auto stream = static_cast<stringstream*>(i.str.get());
                    const auto text = stream->str();
                    stream->str(string());
                    threadLogs+=text;
                }
            }
        }

        if(output.lastWasBrackets)
        {
//            *output.str << '\n';
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

            if(output.lastWasBrackets)
            {
                const auto countOfindention = getWidth()  + fmt.size() + 7 ;
                indent(countOfindention, ' ', ' ');
                printBrackets(str, color);
            }
        }
        else
        {
            resetX();
        }
        if(!threadLogs.empty())
        {
            *this << anounce << threadLogs << ok;
        }

        output.lastWasBrackets = true;
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

Lout::Lout():
             output(mkOutput()),
             bars{"\u2591", "\u2588"},
             fmt("dd.MM.yyyy hh:mm:ss.zzz"),
             width(fmt.size()+1+brWidth+8)
{    
    lastX.push(0); 
    logLevels.push(make_pair(Info,MessageMask(1)));
}

bool Lout::canMessage() const
{
    return logLevels.top().first <= outLevel && outFilterMask & logLevels.top().second;
}

void Lout::popMsgLevel()
{
    if(logLevels.empty())
    {        
        *this << Info
              << '\n'
              << anounce
              << QObject::tr("Wrong message stack balance!")
              << fail;
        exit(-1);
    }    
    logLevels.pop();
}

void Lout::noBr()
{
    output.lastWasBrackets = false;
    hasAnounce = false;
}

void Lout::indent(const size_t cnt, const char inner, const char chr)
{
    if(cnt)
    {
        lock_guard lck(output.mtx);
        *output.str << std::right << std::setfill(inner) << std::setw(cnt) <<  chr;
    }
}

void Lout::flood(size_t cnt, const string& chr)
{
    while(cnt--)
    {
        *this << chr;
    }
}

size_t Lout::strlen(const string_view &in)
{
    size_t ret=0;
    const int len = in.length();
    for(int i=0;i < len;++ret)
    {
        UChar32 c;
        U8_NEXT(in.cbegin(), i, len, c);
    }
    return ret;
}

int Lout::roll(const string_view &in, int i)
{
    const int len=in.length();
    while(i<len)
    {
        UChar32 c;
        U8_NEXT(in.cbegin(), i, len, c);
        if(c=='\n')
        {
            return i;
        }
    }
    return i;
}


int Lout::roll(const string_view &in, int i, size_t pos)
{
    const int len=in.length();
    while(pos--)
    {
        UChar32 c;
        U8_NEXT(in.cbegin(), i, len, c);
    }
    return i;
}

string_view Lout::substr(const string_view &in, const size_t pos)
{
    const size_t ofs = roll(in, 0, pos);
    return string_view( in.cbegin()+ofs, size_t(in.length()-ofs));
}

string_view Lout::substr(const string_view &in, const size_t pos, const size_t count)
{
    const int beg = roll(in, 0,   pos);
    const int  en = roll(in, beg, count);
    return string_view(in.cbegin() + beg, en-beg);
}

void Lout::printW(const string &in, const size_t width, const std::string& filler)
{
    const size_t len = min(strlen(in), width-1);
    print(substr(in, 0, len));
    flood(width - len, filler);
}

Lout& Lout::draw(const Picture &image)
{
    if(image.empty())
    {
        return *this;
    }
    const auto widestLine = max_element(image.cbegin(), image.cend(), [](const Picture::value_type& a,
                                                                         const Picture::value_type& b)
                                                                            {
                                                                                 return a.size() < b.size();
                                                                            }
                                                                        )->size();
    if(!widestLine)
    {
        return *this;
    }
    const auto screenW = getWidth();
    const auto printW = min(screenW, widestLine);
    const auto aspect = max(1.0f, float(widestLine-1) / float(screenW-1));
    //когда картинка шире, aspect > 1, и надо уменьшить высоту, поэтому, делим
    const size_t printH = ceil(image.size() / aspect);

    for(size_t i=0;i<printH;++i)
    {
        const size_t srcY = min( size_t(round (i * aspect)), image.size()-1);
        const auto& pos = image[srcY];
        for(size_t j=0;j<printW;++j)
        {
            const size_t srcX = round (j * aspect);
            const bool hasChar = pos.size()>srcX && !pos[srcX].isEmpty();

            if(hasChar)
            {                
                *this <<pos[srcX];
            }
            else
            {
                print(bars[0]);
            }
        }
        newLine();
    }
    newLine();
    return *this;
}

void Lout::indentLineStart()
{
    indent(fmt.size()+7+getLastX(),' ', ' ');
}

void Lout::newLine()
{
    if(canMessage())
    {
        lock_guard lck(output.mtx);
        resetX();
        *output.str << '\n';
        indentLineStart();
        hasAnounce = true;
        output.lastWasBrackets = false;
    }
}

void Lout::doAnounce()
{
    if(canMessage())
    {
        lock_guard lck(output.mtx);
        *output.str << '\n';
        if( !output.lastWasBrackets || getLastX())
        {
            const auto old = lastX.size();
            const auto cnt = old*4;
            lastX.push(cnt);
            indent((old-1)*4, ' ', ' ');
            *output.str << "\u2514\u2500\u2500\u2500";
        }

        *this<<setColor(36);
        *output.str << '['
             <<  QDateTime::currentDateTime().toString(fmt).toStdString()
             << "]     ";
        *this<<noColor;
        output.lastWasBrackets = false;
        hasAnounce = true;
    }
}

void Lout::preIndent()
{
    if(!hasAnounce && output.lastWasBrackets)
    {
        indentLineStart();
    }
}

void Lout::print(string_view in)
{
    if(canMessage())
    {                      
        lock_guard lck(output.mtx);
        const size_t width=getWidth();  //width of text area

        preIndent();
        noBr();

        for(;;)
        {
            const size_t len=roll(in, 0);
            const bool last = len == in.length();
            const size_t usageLen = len - !last;
            for(size_t i=0;;)
            {
                const size_t j=i;
                const auto oldX = getLastX();
                const auto add=width - oldX;
                i+= add;

                if(i>=usageLen )
                {
                    shift(usageLen-j);
                    *output.str << substr(in, j,usageLen-j);
                    break;
                }
                *output.str << substr(in, j, i-j) << '\n';
                resetX();
                indentLineStart();
            }
            if(last)
            {
                break;
            }
            newLine();
            in=string_view(in.cbegin()+len, in.length()-len);
        }
        *this << flush;
    }
}

Lout &operator <<(Lout &out, const Lout::LogLevel lvl)
{    
    out.logLevels.push( make_pair(lvl, out.logLevels.top().second) );
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
        out << string(1, rhs);
    }
    return out;
}

Lout &flush(Lout &out)
{
    if(out.canMessage())
    {
        lock_guard lck(out.output.mtx);
        *out.output.str << flush;
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

Lout &operator <<(Lout &out, const int32_t rhs)
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


Lout &operator <<(Lout &out, const Lout::PictureElement &rhs)
{
    return out <<setColor(rhs.getColor()) << rhs.getChr() << noColor;
}

Lout &operator <<(Lout &out, const float &rhs)
{
    return out << to_string(rhs);
}

Lout &operator <<(Lout &out, const Lout::Picture &rhs)
{
    return out.draw(rhs);
}

Lout &operator <<(Lout &out, const thread::id &rhs)
{
    stringstream s;
    s << rhs;
    return out << s.str();
}
#if _WIN64 || __x86_64__
Lout &operator <<(Lout &out, const uint32_t rhs)
{
    return out << to_string(rhs);
}
#endif
Lout &operator <<(Lout &out, const Lout::MessageMask &rhs)
{
    out.logLevels.push(make_pair( out.logLevels.top().first, rhs));
    return out;
}

Lout &operator <<(Lout &out, const long &rhs)
{
    out << to_string(rhs);
    return out;
}
