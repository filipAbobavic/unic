
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
 
using namespace std;

// ---------------------------------Токени---------------------------------
enum class TokenType
{
    Number, // число
    Variable, // змінна
    Function, // floor, sqrt
    LParen, // (
    RParen, // )
    BinOp, // бінарні оператори
    UnaryOp, // унарні префікси
    PostfixOp // ++ --
};

struct token
{
    TokenType type;
    string text; // все, шо не число
    double value; //все, шо число
    size_t pos; // позиція (для помилок)
};

struct Errors
{
    string message;
    size_t pos;
};

bool is_digit (char c) { return c >= '0' && c <= '9'; }
bool is_letter (char c) { return c >= 'a' && c <= 'z'; }

// ---------------------------------Токенізація---------------------------------
vector<token> tokenize(const string &s)
{
    vector<token> out;
    
    bool expectOperand = true; // чи очікуємо ми операнд, чи оператор (true - операнд, false - операнд)
    bool afterFunction = false; // чи є дужка після floor/sqrt
    vector<int> openParens; // позиції відкритих дужок
    size_t i = 0;
    
    while ( i < s.size() )
    {
        char c = s[i];
        if ( c == ' ' || c == '\t' || c == '\n' || c == '\r') { i++; continue; } // перевірка на пробіли, табуляцію і всю іншу фігню, яку ми ігноруємо
        
        if (afterFunction && c!='(') throw Errors{"You need to write '(' after the function ", i};
        
        // якщо наш char число
        if (is_digit(c))
        {
            if (!expectOperand) throw Errors{"operator missing before number ", i}; //перевірка на дебіла
            
            size_t j = i;
            while (j < s.size() && is_digit(s[j])) j++; // дивимось, скільки символів підряд є цифрою
            
            if (j < s.size() && (s[j] == '.' || s[j] == ',')) throw Errors{"Check the requirements - fractions aren't allowed ", j};
            
            string num = s.substr(i, j - i);
            double val = stod(num); // перетворюємо наш рядок на чило
            out.push_back({TokenType::Number, num, val, i});
            expectOperand = false;
            i = j;
        }
        
        // якщо наш char змінна або функція
        else if (is_letter(c))
        {
            size_t j = i;
            while (j < s.size() && (is_letter(s[j]) || is_digit(s[j]))) j++; // дивимось, скільки символів підряд є змінною або функцією
            
            string name = s.substr(i, j - i); // відокремлюємо нашу змінну або функцію
            
            if (!expectOperand) throw Errors{"operator omitted before '" + name + "'", i}; //перевірка на розумну людину
            // дивимось, чи це функція
            if (name == "floor" || name == "sqrt") 
            {
                out.push_back({TokenType::Function, name, 0, i});
                afterFunction = true;                       // expectOperand лишається true
            } 
            // якщо це не функція
            else 
            {
                out.push_back({TokenType::Variable, name, 0, i});
                expectOperand = false;
            }
            i = j;
        }
        
        // якщо наш char - '('
        else if (c == '(') 
        {      
            if (!expectOperand) throw Errors{"operator missing before '('", i};
            out.push_back({TokenType::LParen, "(", 0, i});
            openParens.push_back(i);
            afterFunction = false;
            i++;
        }
        
        // якщо наш char - '('
        else if (c == ')') 
        {                                
            // перевірка на відкриваючу дужку
            if (openParens.empty()) throw Errors{"closing parenthesis without an opening one", i};
            
            // перевірка на порожні дужки
            if (expectOperand) {
                bool empty = !out.empty() && out.back().type == TokenType::LParen;
                throw Errors{empty ? "порожні дужки" : "очікується операнд перед ')'", i};
            }
            
            out.push_back({TokenType::RParen, ")", 0, i});
            openParens.pop_back();
            i++;
            
        
        }
        
        // для + - ++ --
        else if (c == '+' || c == '-') 
        { 
            bool dbl = (i + 1 < s.size() && s[i + 1] == c); // два однакові підряд: ++ або --
            
            string two(2, c);
            if (dbl && !expectOperand) {
                // правий ++ -- : лише одразу після змінної
                if (out.back().type != TokenType::Variable) throw Errors{"'" + two + "' can be applied only to a variable ", i}; // перевірка на синтаксиз
                out.push_back({TokenType::PostfixOp, two, 0, i});
                i += 2;
            } else if (expectOperand) {
                if (dbl) throw Errors{"left '" + two + "' is not supported", i}; // перевірка на правий/лівий
                out.push_back({TokenType::UnaryOp, string("u") + c, 0, i});
                i++;
                // expectOperand лишається true
            } else {
                out.push_back({TokenType::BinOp, string(1, c), 0, i});
                expectOperand = true;
                i++;
            }
        }
        
        // для *
        else if (c == '*') 
        {                                
            if (expectOperand) throw Errors{"оператор '*' не має лівого операнда", i};
            out.push_back({TokenType::BinOp, "*", 0, i});
            expectOperand = true;
            i++;
        }
        
        //для всього, що не є у білих списках
        else 
        {
            throw Errors{string("unknown symbol '") + c + "'", i};
        }
    }
     // ще парочку помилок
    if (afterFunction) throw Errors{"'(' expected after function", s.size()};
    if (expectOperand) throw Errors{out.empty() ? "blank expression" : "operand expected at the end of the expression", s.size()};
    if (!openParens.empty()) throw Errors{"missing closing parenthesis for '('", size_t(openParens.back())};
    
    return out;
}

// ---------------------------------Перетворення у зворотню польську нотацію---------------------------------
//Додамо пріорітетність діям (для ++ -- не треба, бо пріорітет найвищий)
int priority(const token& t) {
    if (t.type == TokenType::UnaryOp) return 3;
    if (t.text == "*") return 2;
    return 1;
}

// зворотня польська нотація
vector <token> ZPN(const vector<token>& in)
{
    vector<token> out;
    stack<token> st;
    
    for (const token& t : in) {
        switch (t.type) {
        case TokenType::Number:
        case TokenType::Variable:
        case TokenType::PostfixOp:
            // Правий ++/-- має найвищий пріоритет і стосується найближчого операнда ліворуч, тому йде одразу у вихід
            out.push_back(t);
            break;
 
        case TokenType::Function:
        case TokenType::LParen:
        case TokenType::UnaryOp:
            // Функцію та '(' закидаємо в стек. Префіксний унарний оператор право-асоціативний і не має лівого операнда - теж просто в стек
            st.push(t);
            break;
 
        case TokenType::BinOp:
            // Бінарні оператори ліво-асоціативні: поки пріоритет оператора зверху стека >= поточного - виштовхуємо його у вихід
            while (!st.empty() &&
                   (st.top().type == TokenType::UnaryOp || st.top().type == TokenType::BinOp) && priority(st.top()) >= priority(t)) 
                {
                out.push_back(st.top());
                st.pop();
                }
            st.push(t);
            break;
 
        case TokenType::RParen:
            // Виштовхуємо все до '(', саму '(' викидаємо.
            while (!st.empty() && st.top().type != TokenType::LParen) 
            {
                out.push_back(st.top());
                st.pop();
            }
            if (!st.empty()) st.pop();
            // Якщо під дужкою була функція — виштовхуємо її у вихід.
            if (!st.empty() && st.top().type == TokenType::Function) 
            {
                out.push_back(st.top());
                st.pop();
            }
            break;
        }
    }
    // вхідний рядок закінчився
    while (!st.empty()) 
    {            
        
        out.push_back(st.top());
        st.pop();
    }
    return out;
}

// ---------------------------------Обчислення зворотньої польської нотації---------------------------------

string fmt(double x) {
    if (x == 0) x = 0; // шоб не писати "-0"
    ostringstream o;
    o << setprecision(10) << x;
    return o.str();
}
 
// Елемент стека обчислень: значення + ім'я змінної (треба тіки для ++/--)
struct Val {
    double v;
    string var;
};
 
double evaluate(const vector<token>& rpn, map<string, double>& vars) {
    vector<Val> st;
    
    auto pop = [&]() -> Val 
    {
        Val v = st.back();
        st.pop_back();
        return v;
    };
 
    cout << endl << "Step | Element | Stack" << endl;
    int step = 0;
 
    for (const token& t : rpn) {
        switch (t.type) {
        
        // для числа
        case TokenType::Number:
            st.push_back({t.value, ""});
            break;
 
        // для змінної
        case TokenType::Variable:
            st.push_back({vars[t.text], t.text});
            break;
 
        // для бінарного
        case TokenType::BinOp: 
            {
            double x1 = pop().v; // другий операнд
            double x2 = pop().v; // перший операнд
            double r = (t.text == "+") ? x2 + x1
                     : (t.text == "-") ? x2 - x1
                                       : x2 * x1; // дія x2 A x1
            st.push_back({r, ""});
            break;
        }
    
        // для унарного
        case TokenType::UnaryOp: 
        {
            double x = pop().v;
            st.push_back({t.text == "u-" ? -x : x, ""});
            break;
        }
        
        // для ++ --
        case TokenType::PostfixOp: 
        {
            Val x = pop();
            double old = vars[x.var];
            vars[x.var] = old + (t.text == "++" ? 1 : -1);   // змінюємо змінну
            st.push_back({old, ""}); // результат - старе значення
            break;
        }
 
        case TokenType::Function: 
        {
            double x = pop().v;
            if (t.text == "floor") st.push_back({floor(x), ""});
            else 
            {
                if (x < 0) throw runtime_error("sqrt of a negative number");
                st.push_back({sqrt(x), ""});
            }
            break;
        }
 
        default:
            throw runtime_error("internal error: parenthesis in RPN");
        }
 
        // друк рядка таблиці (чисто по приколу)
        cout << ++step << " | " << t.text << " |";
        for (const Val& v : st) cout << ' ' << fmt(v.v);
        cout << endl;
    }
    return pop().v;
}

//---------------------------------Додеп функції---------------------------------

// перетворює ОПН у рядок
string zpnToString(const vector<token>& zpn)
{
    string s;
    for (const token& t : zpn) s += t.text + " ";
    return s;
}

// введення змінних та перевірка їх
// повертає false, якщо ввід закінчився
bool askValue(const string& name, double& value)
{
    while (true)
    {
        cout << "Enter the value of " << name << " (integer >= 0): ";
        string line;
        if (!getline(cin, line)) return false;
 
        // прибираємо пробіли з країв
        size_t b = line.find_first_not_of(" \t\r");
        size_t e = line.find_last_not_of(" \t\r");
        if (b == string::npos) { cout << "  Empty input, try again." << endl; continue; }
        line = line.substr(b, e - b + 1);
 
        // перевірка: тільки цифри і не надто довге число
        bool ok = line.size() <= 15;
        for (char c : line) if (!is_digit(c)) ok = false;
        if (!ok) { cout << "  Not a valid value, try again." << endl; continue; }
 
        value = stod(line);
        return true;
    }
}

int main()
{
    cout << "Variant 3" << endl;
    cout << "Operands: integers >= 0 and variables (lowercase latin letters and digits, start with a letter)" << endl;
    cout << "Binary: + - *   Unary: + -   Postfix: ++ --   Functions: floor(...), sqrt(...)" << endl;
    cout << "An empty line exits the program" << endl;
 
    while (true)
    {
        cout << endl << "Enter expression: ";
        string line;
        if (!getline(cin, line)) break; // кінець вводу
        if (line.find_first_not_of(" \t\r") == string::npos) break; // порожній рядок - вихід
 
        try
        {
            // 1) токенізація + перевірка правильності
            vector<token> infix = tokenize(line);
 
            // 2) переведення в ОПН
            vector<token> zpn = ZPN(infix);
            cout << "ZPN: " << zpnToString(zpn) << endl;
 
            // 3) значення змінних: питаємо по одному разу для кожної різної змінної
            map<string, double> vars;
            for (const token& t : infix)
            {
                if (t.type == TokenType::Variable && !vars.count(t.text))
                {
                    double v;
                    if (!askValue(t.text, v)) return 0;
                    vars[t.text] = v;
                }
            }
 
            // 4) обчислення
            double result = evaluate(zpn, vars);
            cout << endl << "Result: " << fmt(result) << endl;
 
            // якщо були ++ або --, показуємо нові значення змінних
            bool changed = false;
            for (const token& t : infix)
                if (t.type == TokenType::PostfixOp) changed = true;
            if (changed)
            {
                cout << "Variables after evaluation:";
                for (const auto& kv : vars) cout << "  " << kv.first << " = " << fmt(kv.second);
                cout << endl;
            }
        }
        // помилка у виразі (кидає tokenize): лише факт помилки і номер елемента
        catch (const Errors& e)
        {
            cout << "Error in element " << e.pos + 1 << endl;
        }
        catch (const runtime_error& e)
        {
            cout << "Error: " << e.what() << endl;
        }
        // наприклад, stod для надто великого числа
        catch (const exception&)
        {
            cout << "Error: number is too large" << endl;
        }
    }
    return 0;
}