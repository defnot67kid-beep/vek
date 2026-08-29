#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;

namespace {
std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size()+8);
    for(char c:s) {
        switch(c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}
std::string ToJsonDepth(const VekValue& v, int depth) {
    if(depth>24) return "null";
    if(v.IsNil()) return "null";
    if(v.IsBool()) return v.AsBool()?"true":"false";
    if(v.IsNumber()) { std::ostringstream o; o.precision(15); o<<v.AsNumber(); return o.str(); }
    if(v.IsString()) return "\""+JsonEscape(v.AsString())+"\"";
    if(v.IsArray()) {
        std::string out="["; const auto* a=v.AsArray();
        for(std::size_t i=0;a&&i<a->size();++i){if(i)out+=",";out+=ToJsonDepth((*a)[i],depth+1);}return out+"]";
    }
    if(v.IsMap()) {
        std::vector<std::string> keys; const auto* m=v.AsMap(); if(m){keys.reserve(m->size());for(auto&kv:*m)keys.push_back(kv.first);}std::sort(keys.begin(),keys.end());
        std::string out="{"; for(std::size_t i=0;i<keys.size();++i){if(i)out+=",";out+="\""+JsonEscape(keys[i])+"\":"+ToJsonDepth(m->at(keys[i]),depth+1);}return out+"}";
    }
    return "null";
}
}

VekValue VekValue::Array(){return VekValue(VekArray{});} 
VekValue VekValue::Map(){return VekValue(VekMap{});} 
bool VekValue::IsNil() const{return std::holds_alternative<std::monostate>(value);} 
bool VekValue::IsNumber() const{return std::holds_alternative<double>(value);} 
bool VekValue::IsBool() const{return std::holds_alternative<bool>(value);} 
bool VekValue::IsString() const{return std::holds_alternative<std::string>(value);} 
bool VekValue::IsArray() const{return std::holds_alternative<ArrayPtr>(value);} 
bool VekValue::IsMap() const{return std::holds_alternative<MapPtr>(value);} 

double VekValue::AsNumber(double fallback) const {
    if(auto p=std::get_if<double>(&value))return *p;
    if(auto p=std::get_if<bool>(&value))return *p?1.0:0.0;
    if(auto p=std::get_if<std::string>(&value)){try{std::size_t n=0;double x=std::stod(*p,&n);return n==p->size()?x:fallback;}catch(...){return fallback;}}
    return fallback;
}
bool VekValue::AsBool(bool fallback) const {
    if(auto p=std::get_if<bool>(&value))return *p;
    if(auto p=std::get_if<double>(&value))return std::fabs(*p)>1e-12;
    if(auto p=std::get_if<std::string>(&value))return !p->empty();
    if(IsArray())return Size()!=0;
    if(IsMap())return Size()!=0;
    if(IsNil())return false;
    return fallback;
}
std::string VekValue::AsString() const {
    if(auto p=std::get_if<std::string>(&value))return *p;
    if(auto p=std::get_if<bool>(&value))return *p?"true":"false";
    if(auto p=std::get_if<double>(&value)){std::ostringstream o;o.precision(15);o<<*p;return o.str();}
    if(IsArray()||IsMap())return ToJson();
    return "nil";
}
bool VekValue::Truthy() const{return AsBool(false);} 
std::size_t VekValue::Size() const {if(auto p=std::get_if<std::string>(&value))return p->size();if(auto p=std::get_if<ArrayPtr>(&value))return *p?(*p)->size():0;if(auto p=std::get_if<MapPtr>(&value))return *p?(*p)->size():0;return 0;}
const VekArray* VekValue::AsArray() const{if(auto p=std::get_if<ArrayPtr>(&value))return p->get();return nullptr;} 
VekArray* VekValue::AsArray(){if(auto p=std::get_if<ArrayPtr>(&value))return p->get();return nullptr;} 
const VekMap* VekValue::AsMap() const{if(auto p=std::get_if<MapPtr>(&value))return p->get();return nullptr;} 
VekMap* VekValue::AsMap(){if(auto p=std::get_if<MapPtr>(&value))return p->get();return nullptr;} 
VekValue VekValue::Get(const std::string& key) const{auto*m=AsMap();if(!m)return {};auto it=m->find(key);return it==m->end()?VekValue():it->second;} 
VekValue VekValue::Get(std::size_t i) const{auto*a=AsArray();return (!a||i>=a->size())?VekValue():(*a)[i];} 
bool VekValue::Set(const std::string& key,VekValue v){auto*m=AsMap();if(!m)return false;(*m)[key]=std::move(v);return true;} 
bool VekValue::Set(std::size_t i,VekValue v){auto*a=AsArray();if(!a||i>=a->size())return false;(*a)[i]=std::move(v);return true;} 
bool VekValue::Push(VekValue v){auto*a=AsArray();if(!a)return false;a->push_back(std::move(v));return true;} 
std::string VekValue::ToJson() const{return ToJsonDepth(*this,0);} 

namespace {

enum class TokenType {
    End, Identifier, Number, String,
    Fn, If, Else, Let, While, Break, Continue, Return, True, False, Nil,
    Struct, Event,
    LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
    Comma, Semicolon, Colon, Dot,
    Plus, Minus, Star, Slash, Percent,
    Bang, BangEqual, Equal, EqualEqual,
    Greater, GreaterEqual, Less, LessEqual,
    AndAnd, OrOr
};
struct Token{TokenType type=TokenType::End;std::string lexeme;double number=0;int line=1;};

class Lexer {
public:
    Lexer(const std::string&s,const VekSecurityPolicy&p):source(s),policy(p){}
    std::vector<Token> Scan(){std::vector<Token>o;while(!AtEnd()){start=current;ScanToken(o);if(o.size()>policy.maxTokens)throw std::runtime_error("VEK security: token count exceeds limit");}o.push_back({TokenType::End,"",0,line});return o;}
private:
    const std::string&source;const VekSecurityPolicy&policy;std::size_t start=0,current=0;int line=1;
    bool AtEnd()const{return current>=source.size();}char Advance(){return source[current++];}char Peek()const{return AtEnd()?'\0':source[current];}char PeekNext()const{return current+1>=source.size()?'\0':source[current+1];}
    bool Match(char e){if(AtEnd()||source[current]!=e)return false;++current;return true;}void Add(std::vector<Token>&o,TokenType t){o.push_back({t,source.substr(start,current-start),0,line});}
    [[noreturn]]void Error(const std::string&m)const{throw std::runtime_error("VEK lexer line "+std::to_string(line)+": "+m);} 
    void ScanToken(std::vector<Token>&o){char c=Advance();switch(c){
        case '(':Add(o,TokenType::LeftParen);break;case ')':Add(o,TokenType::RightParen);break;case '{':Add(o,TokenType::LeftBrace);break;case '}':Add(o,TokenType::RightBrace);break;
        case '[':Add(o,TokenType::LeftBracket);break;case ']':Add(o,TokenType::RightBracket);break;case ',':Add(o,TokenType::Comma);break;case ';':Add(o,TokenType::Semicolon);break;case ':':Add(o,TokenType::Colon);break;case '.':Add(o,TokenType::Dot);break;
        case '+':Add(o,TokenType::Plus);break;case '-':Add(o,TokenType::Minus);break;case '*':Add(o,TokenType::Star);break;case '%':Add(o,TokenType::Percent);break;
        case '!':Add(o,Match('=')?TokenType::BangEqual:TokenType::Bang);break;case '=':Add(o,Match('=')?TokenType::EqualEqual:TokenType::Equal);break;
        case '>':Add(o,Match('=')?TokenType::GreaterEqual:TokenType::Greater);break;case '<':Add(o,Match('=')?TokenType::LessEqual:TokenType::Less);break;
        case '&':if(Match('&'))Add(o,TokenType::AndAnd);else Error("expected '&' after '&'");break;case '|':if(Match('|'))Add(o,TokenType::OrOr);else Error("expected '|' after '|'");break;
        case '/':if(Match('/')){while(Peek()!='\n'&&!AtEnd())Advance();}else Add(o,TokenType::Slash);break;case '#':while(Peek()!='\n'&&!AtEnd())Advance();break;
        case ' ':case '\r':case '\t':break;case '\n':++line;break;case '"':String(o);break;
        default:if(std::isdigit((unsigned char)c))Number(o);else if(std::isalpha((unsigned char)c)||c=='_')Identifier(o);else Error("unexpected character");break;}}
    void String(std::vector<Token>&o){std::string v;while(!AtEnd()&&Peek()!='"'){char c=Advance();if(c=='\n')++line;if(c=='\\'&&!AtEnd()){char e=Advance();switch(e){case 'n':v+='\n';break;case 'r':v+='\r';break;case 't':v+='\t';break;case '"':v+='"';break;case '\\':v+='\\';break;default:v+=e;}}else v+=c;if(v.size()>policy.maxStringBytes)Error("string literal exceeds configured limit");}if(AtEnd())Error("unterminated string");Advance();o.push_back({TokenType::String,v,0,line});}
    void Number(std::vector<Token>&o){while(std::isdigit((unsigned char)Peek()))Advance();if(Peek()=='.'&&std::isdigit((unsigned char)PeekNext())){Advance();while(std::isdigit((unsigned char)Peek()))Advance();}std::string t=source.substr(start,current-start);o.push_back({TokenType::Number,t,std::stod(t),line});}
    void Identifier(std::vector<Token>&o){while(std::isalnum((unsigned char)Peek())||Peek()=='_')Advance();std::string t=source.substr(start,current-start);static const std::unordered_map<std::string,TokenType>w={{"fn",TokenType::Fn},{"if",TokenType::If},{"else",TokenType::Else},{"let",TokenType::Let},{"while",TokenType::While},{"break",TokenType::Break},{"continue",TokenType::Continue},{"return",TokenType::Return},{"true",TokenType::True},{"false",TokenType::False},{"nil",TokenType::Nil},{"struct",TokenType::Struct},{"event",TokenType::Event}};auto it=w.find(t);o.push_back({it==w.end()?TokenType::Identifier:it->second,t,0,line});}
};

struct Expr {
    enum class Kind{Literal,Variable,Unary,Binary,Call,Array,Object,Index,Member,StructInit}kind=Kind::Literal;
    VekValue literal;std::string text;TokenType op=TokenType::End;std::unique_ptr<Expr>left,right;std::vector<std::unique_ptr<Expr>>args;std::vector<std::pair<std::string,std::unique_ptr<Expr>>>fields;
};
struct Stmt {enum class Kind{Expression,Let,Assign,If,While,Break,Continue,Return}kind=Kind::Expression;std::string name;std::unique_ptr<Expr>expression;std::vector<Stmt>thenBranch,elseBranch;};
struct FunctionDef{std::vector<std::string>params;std::vector<Stmt>body;};
struct Program{std::unordered_map<std::string,FunctionDef>functions;std::unordered_map<std::string,std::vector<std::string>>structs;std::unordered_set<std::string>events;};

class Parser {
public:
    Parser(std::vector<Token>in,const VekSecurityPolicy&p):tokens(std::move(in)),policy(p){}
    Program ParseProgram(){Program out;while(!Check(TokenType::End)){
        if(Match(TokenType::Struct)){ParseStruct(out);continue;}
        bool event=false;if(Match(TokenType::Event))event=true;else Consume(TokenType::Fn,"expected 'fn', 'event', or 'struct'");
        if(out.functions.size()>=policy.maxFunctions)Error("function count exceeds limit");Token name=Consume(TokenType::Identifier,"expected function/event name");FunctionDef fn=ParseFunctionTail();std::string stored=event?"__event_"+name.lexeme:name.lexeme;out.functions[stored]=std::move(fn);if(event)out.events.insert(name.lexeme);
    }return out;}
private:
    std::vector<Token>tokens;const VekSecurityPolicy&policy;std::size_t current=0;
    bool Check(TokenType t)const{return tokens[current].type==t;}bool CheckNext(TokenType t)const{return current+1<tokens.size()&&tokens[current+1].type==t;}bool AtEnd()const{return Check(TokenType::End);}Token Advance(){if(!AtEnd())++current;return tokens[current-1];}bool Match(TokenType t){if(!Check(t))return false;Advance();return true;}[[noreturn]]void Error(const std::string&m)const{throw std::runtime_error("VEK parser line "+std::to_string(tokens[current].line)+": "+m);}Token Consume(TokenType t,const std::string&m){if(Check(t))return Advance();Error(m);}
    void ParseStruct(Program&out){Token name=Consume(TokenType::Identifier,"expected struct name");Consume(TokenType::LeftBrace,"expected '{'");std::vector<std::string>fields;while(!Check(TokenType::RightBrace)&&!AtEnd()){fields.push_back(Consume(TokenType::Identifier,"expected field name").lexeme);Consume(TokenType::Semicolon,"expected ';' after struct field");if(fields.size()>256)Error("too many struct fields");}Consume(TokenType::RightBrace,"expected '}'");Match(TokenType::Semicolon);out.structs[name.lexeme]=std::move(fields);}
    FunctionDef ParseFunctionTail(){Consume(TokenType::LeftParen,"expected '('");FunctionDef fn;if(!Check(TokenType::RightParen)){do{if(fn.params.size()>=policy.maxParametersPerFunction)Error("too many function parameters");fn.params.push_back(Consume(TokenType::Identifier,"expected parameter name").lexeme);}while(Match(TokenType::Comma));}Consume(TokenType::RightParen,"expected ')'");fn.body=ParseBlock();return fn;}
    std::vector<Stmt>ParseBlock(){Consume(TokenType::LeftBrace,"expected '{'");std::vector<Stmt>b;while(!Check(TokenType::RightBrace)&&!AtEnd())b.push_back(ParseStatement());Consume(TokenType::RightBrace,"expected '}'");return b;}
    Stmt ParseStatement(){if(Match(TokenType::If))return ParseIf();if(Match(TokenType::While))return ParseWhile();if(Match(TokenType::Let))return ParseLet();if(Match(TokenType::Break)){Stmt s;s.kind=Stmt::Kind::Break;Consume(TokenType::Semicolon,"expected ';'");return s;}if(Match(TokenType::Continue)){Stmt s;s.kind=Stmt::Kind::Continue;Consume(TokenType::Semicolon,"expected ';'");return s;}if(Match(TokenType::Return))return ParseReturn();if(Check(TokenType::Identifier)&&CheckNext(TokenType::Equal))return ParseAssign();Stmt s;s.kind=Stmt::Kind::Expression;s.expression=ParseExpression();Consume(TokenType::Semicolon,"expected ';' after expression");return s;}
    Stmt ParseIf(){Stmt s;s.kind=Stmt::Kind::If;s.expression=ParseExpression();s.thenBranch=ParseBlock();if(Match(TokenType::Else)){if(Match(TokenType::If)){Stmt n=ParseIf();s.elseBranch.push_back(std::move(n));}else s.elseBranch=ParseBlock();}return s;}
    Stmt ParseWhile(){Stmt s;s.kind=Stmt::Kind::While;s.expression=ParseExpression();s.thenBranch=ParseBlock();return s;}
    Stmt ParseLet(){Stmt s;s.kind=Stmt::Kind::Let;s.name=Consume(TokenType::Identifier,"expected variable name").lexeme;Consume(TokenType::Equal,"expected '='");s.expression=ParseExpression();Consume(TokenType::Semicolon,"expected ';'");return s;}
    Stmt ParseAssign(){Stmt s;s.kind=Stmt::Kind::Assign;s.name=Advance().lexeme;Consume(TokenType::Equal,"expected '='");s.expression=ParseExpression();Consume(TokenType::Semicolon,"expected ';'");return s;}
    Stmt ParseReturn(){Stmt s;s.kind=Stmt::Kind::Return;if(!Check(TokenType::Semicolon))s.expression=ParseExpression();Consume(TokenType::Semicolon,"expected ';'");return s;}
    std::unique_ptr<Expr>ParseExpression(){return ParseOr();}
    std::unique_ptr<Expr>ParseOr(){auto e=ParseAnd();while(Match(TokenType::OrOr))e=Binary(std::move(e),TokenType::OrOr,ParseAnd());return e;}
    std::unique_ptr<Expr>ParseAnd(){auto e=ParseEquality();while(Match(TokenType::AndAnd))e=Binary(std::move(e),TokenType::AndAnd,ParseEquality());return e;}
    std::unique_ptr<Expr>ParseEquality(){auto e=ParseComparison();while(Check(TokenType::EqualEqual)||Check(TokenType::BangEqual)){auto o=Advance().type;e=Binary(std::move(e),o,ParseComparison());}return e;}
    std::unique_ptr<Expr>ParseComparison(){auto e=ParseTerm();while(Check(TokenType::Greater)||Check(TokenType::GreaterEqual)||Check(TokenType::Less)||Check(TokenType::LessEqual)){auto o=Advance().type;e=Binary(std::move(e),o,ParseTerm());}return e;}
    std::unique_ptr<Expr>ParseTerm(){auto e=ParseFactor();while(Check(TokenType::Plus)||Check(TokenType::Minus)){auto o=Advance().type;e=Binary(std::move(e),o,ParseFactor());}return e;}
    std::unique_ptr<Expr>ParseFactor(){auto e=ParseUnary();while(Check(TokenType::Star)||Check(TokenType::Slash)||Check(TokenType::Percent)){auto o=Advance().type;e=Binary(std::move(e),o,ParseUnary());}return e;}
    std::unique_ptr<Expr>ParseUnary(){if(Check(TokenType::Bang)||Check(TokenType::Minus)){auto o=Advance().type;auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Unary;e->op=o;e->right=ParseUnary();return e;}return ParsePostfix();}
    std::unique_ptr<Expr>ParsePostfix(){auto e=ParsePrimary();for(;;){if(Match(TokenType::LeftBracket)){auto idx=ParseExpression();Consume(TokenType::RightBracket,"expected ']'");auto n=std::make_unique<Expr>();n->kind=Expr::Kind::Index;n->left=std::move(e);n->right=std::move(idx);e=std::move(n);}else if(Match(TokenType::Dot)){auto key=Consume(TokenType::Identifier,"expected member name");auto n=std::make_unique<Expr>();n->kind=Expr::Kind::Member;n->left=std::move(e);n->text=key.lexeme;e=std::move(n);}else break;}return e;}
    std::unique_ptr<Expr>ParsePrimary(){
        if(Match(TokenType::True))return Literal(VekValue(true));if(Match(TokenType::False))return Literal(VekValue(false));if(Match(TokenType::Nil))return Literal(VekValue());if(Check(TokenType::Number))return Literal(VekValue(Advance().number));if(Check(TokenType::String))return Literal(VekValue(Advance().lexeme));
        if(Match(TokenType::LeftBracket)){auto a=std::make_unique<Expr>();a->kind=Expr::Kind::Array;if(!Check(TokenType::RightBracket)){do{if(a->args.size()>=policy.maxContainerItems)Error("array literal exceeds item limit");a->args.push_back(ParseExpression());}while(Match(TokenType::Comma));}Consume(TokenType::RightBracket,"expected ']'");return a;}
        if(Match(TokenType::LeftBrace)){auto o=std::make_unique<Expr>();o->kind=Expr::Kind::Object;ParseObjectFields(*o);return o;}
        if(Check(TokenType::Identifier)){Token name=Advance();if(Match(TokenType::LeftParen)){auto c=std::make_unique<Expr>();c->kind=Expr::Kind::Call;c->text=name.lexeme;if(!Check(TokenType::RightParen)){do{if(c->args.size()>=policy.maxArgumentsPerCall)Error("too many call arguments");c->args.push_back(ParseExpression());}while(Match(TokenType::Comma));}Consume(TokenType::RightParen,"expected ')'");return c;}bool structInit=false;if(Check(TokenType::LeftBrace)){if(current+1<tokens.size()&&tokens[current+1].type==TokenType::RightBrace)structInit=true;else if(current+2<tokens.size()&&(tokens[current+1].type==TokenType::Identifier||tokens[current+1].type==TokenType::String)&&tokens[current+2].type==TokenType::Colon)structInit=true;}if(structInit){Advance();auto o=std::make_unique<Expr>();o->kind=Expr::Kind::StructInit;o->text=name.lexeme;ParseObjectFields(*o);return o;}auto v=std::make_unique<Expr>();v->kind=Expr::Kind::Variable;v->text=name.lexeme;return v;}
        if(Match(TokenType::LeftParen)){auto e=ParseExpression();Consume(TokenType::RightParen,"expected ')'");return e;}Error("expected expression");}
    void ParseObjectFields(Expr&o){if(!Check(TokenType::RightBrace)){do{std::string key;if(Check(TokenType::Identifier)||Check(TokenType::String))key=Advance().lexeme;else Error("expected object key");Consume(TokenType::Colon,"expected ':' after object key");o.fields.emplace_back(key,ParseExpression());if(o.fields.size()>policy.maxContainerItems)Error("map literal exceeds item limit");}while(Match(TokenType::Comma));}Consume(TokenType::RightBrace,"expected '}'");}
    static std::unique_ptr<Expr>Literal(VekValue v){auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Literal;e->literal=std::move(v);return e;}
    static std::unique_ptr<Expr>Binary(std::unique_ptr<Expr>l,TokenType o,std::unique_ptr<Expr>r){auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Binary;e->left=std::move(l);e->right=std::move(r);e->op=o;return e;}
};

bool ValuesEqual(const VekValue&a,const VekValue&b){if(a.IsNil()||b.IsNil())return a.IsNil()&&b.IsNil();if(a.IsNumber()&&b.IsNumber())return std::fabs(a.AsNumber()-b.AsNumber())<1e-9;if(a.IsBool()||b.IsBool())return a.AsBool()==b.AsBool();if(a.IsArray()||a.IsMap()||b.IsArray()||b.IsMap())return a.ToJson()==b.ToJson();return a.AsString()==b.AsString();}

std::string Trim(std::string s){auto notsp=[](unsigned char c){return !std::isspace(c);};s.erase(s.begin(),std::find_if(s.begin(),s.end(),notsp));s.erase(std::find_if(s.rbegin(),s.rend(),notsp).base(),s.end());return s;}
bool ParseImportLine(const std::string&line,std::string&spec){std::string t=Trim(line);if(t.rfind("import",0)!=0)return false;std::size_t q1=t.find('"');if(q1==std::string::npos)return false;std::size_t q2=t.find('"',q1+1);if(q2==std::string::npos)return false;spec=t.substr(q1+1,q2-q1-1);std::string tail=Trim(t.substr(q2+1));if(tail!=";")return false;return true;}
bool PathInside(const fs::path&p,const fs::path&root){auto pp=fs::weakly_canonical(p),rr=fs::weakly_canonical(root);auto pit=pp.begin(),rit=rr.begin();for(;rit!=rr.end();++rit,++pit)if(pit==pp.end()||*pit!=*rit)return false;return true;}

} // namespace

struct VekScriptEngine::Impl {
    std::unordered_map<std::string,FunctionDef>functions;std::unordered_map<std::string,std::vector<std::string>>structs;std::unordered_set<std::string>events;std::unordered_map<std::string,NativeFunction>natives;
    std::string lastError,sourceName;bool loaded=false,nativesSealed=false;VekSecurityPolicy policy;std::vector<std::string>moduleRoots;
    std::size_t instructionsRemaining=0,nativeCallsRemaining=0,callDepth=0,loopIterationsRemaining=0;
    using Environment=std::unordered_map<std::string,VekValue>;enum class Flow{Normal,Return,Break,Continue};struct ExecResult{Flow flow=Flow::Normal;VekValue value;};
    void ConsumeInstruction(){if(instructionsRemaining==0)throw std::runtime_error("VEK security: instruction budget exceeded");--instructionsRemaining;}
    VekValue Eval(const Expr&e,Environment&env){ConsumeInstruction();switch(e.kind){
        case Expr::Kind::Literal:return e.literal;
        case Expr::Kind::Variable:{auto it=env.find(e.text);if(it==env.end())throw std::runtime_error("VEK runtime: unknown variable '"+e.text+"'");return it->second;}
        case Expr::Kind::Unary:{auto r=Eval(*e.right,env);if(e.op==TokenType::Bang)return VekValue(!r.Truthy());if(e.op==TokenType::Minus)return VekValue(-r.AsNumber());return {};}
        case Expr::Kind::Binary:{if(e.op==TokenType::AndAnd){auto l=Eval(*e.left,env);return l.Truthy()?VekValue(Eval(*e.right,env).Truthy()):VekValue(false);}if(e.op==TokenType::OrOr){auto l=Eval(*e.left,env);return l.Truthy()?VekValue(true):VekValue(Eval(*e.right,env).Truthy());}auto l=Eval(*e.left,env),r=Eval(*e.right,env);switch(e.op){case TokenType::Plus:if(l.IsString()||r.IsString())return VekValue(l.AsString()+r.AsString());return VekValue(l.AsNumber()+r.AsNumber());case TokenType::Minus:return VekValue(l.AsNumber()-r.AsNumber());case TokenType::Star:return VekValue(l.AsNumber()*r.AsNumber());case TokenType::Slash:{double d=r.AsNumber();if(std::fabs(d)<1e-12)throw std::runtime_error("VEK runtime: division by zero");return VekValue(l.AsNumber()/d);}case TokenType::Percent:{double d=r.AsNumber();if(std::fabs(d)<1e-12)throw std::runtime_error("VEK runtime: modulo by zero");return VekValue(std::fmod(l.AsNumber(),d));}case TokenType::EqualEqual:return VekValue(ValuesEqual(l,r));case TokenType::BangEqual:return VekValue(!ValuesEqual(l,r));case TokenType::Greater:return VekValue(l.AsNumber()>r.AsNumber());case TokenType::GreaterEqual:return VekValue(l.AsNumber()>=r.AsNumber());case TokenType::Less:return VekValue(l.AsNumber()<r.AsNumber());case TokenType::LessEqual:return VekValue(l.AsNumber()<=r.AsNumber());default:return {};}}
        case Expr::Kind::Call:{std::vector<VekValue>a;a.reserve(e.args.size());for(auto&x:e.args)a.push_back(Eval(*x,env));auto n=natives.find(e.text);if(n!=natives.end()){if(nativeCallsRemaining==0)throw std::runtime_error("VEK security: native-call budget exceeded");--nativeCallsRemaining;return n->second(a);}return CallFunction(e.text,a);}
        case Expr::Kind::Array:{VekArray a;a.reserve(e.args.size());for(auto&x:e.args)a.push_back(Eval(*x,env));return VekValue(std::move(a));}
        case Expr::Kind::Object:{VekMap m;for(auto&f:e.fields)m[f.first]=Eval(*f.second,env);return VekValue(std::move(m));}
        case Expr::Kind::Index:{auto base=Eval(*e.left,env),idx=Eval(*e.right,env);if(base.IsArray()){double n=idx.AsNumber(-1);if(n<0)return {};return base.Get((std::size_t)n);}if(base.IsMap())return base.Get(idx.AsString());if(base.IsString()){auto s=base.AsString();auto n=(std::size_t)std::max(0.0,idx.AsNumber());return n<s.size()?VekValue(std::string(1,s[n])):VekValue();}return {};}
        case Expr::Kind::Member:{auto base=Eval(*e.left,env);return base.Get(e.text);}
        case Expr::Kind::StructInit:{auto it=structs.find(e.text);if(it==structs.end())throw std::runtime_error("VEK runtime: unknown struct '"+e.text+"'");VekMap m;m["__type"]=VekValue(e.text);for(auto&field:it->second)m[field]=VekValue();for(auto&f:e.fields){if(std::find(it->second.begin(),it->second.end(),f.first)==it->second.end())throw std::runtime_error("VEK runtime: struct '"+e.text+"' has no field '"+f.first+"'");m[f.first]=Eval(*f.second,env);}return VekValue(std::move(m));}
    }return {};}
    ExecResult ExecuteBlock(const std::vector<Stmt>&body,Environment&env,bool inLoop=false){for(auto&s:body){ConsumeInstruction();switch(s.kind){case Stmt::Kind::Expression:Eval(*s.expression,env);break;case Stmt::Kind::Let:env[s.name]=Eval(*s.expression,env);break;case Stmt::Kind::Assign:{auto it=env.find(s.name);if(it==env.end())throw std::runtime_error("VEK runtime: assignment to unknown variable '"+s.name+"'");it->second=Eval(*s.expression,env);break;}case Stmt::Kind::If:{auto r=ExecuteBlock(Eval(*s.expression,env).Truthy()?s.thenBranch:s.elseBranch,env,inLoop);if(r.flow!=Flow::Normal)return r;break;}case Stmt::Kind::While:while(Eval(*s.expression,env).Truthy()){if(loopIterationsRemaining==0)throw std::runtime_error("VEK security: loop iteration budget exceeded");--loopIterationsRemaining;auto r=ExecuteBlock(s.thenBranch,env,true);if(r.flow==Flow::Return)return r;if(r.flow==Flow::Break)break;if(r.flow==Flow::Continue)continue;}break;case Stmt::Kind::Break:if(!inLoop)throw std::runtime_error("VEK runtime: break outside loop");return {Flow::Break,{}};case Stmt::Kind::Continue:if(!inLoop)throw std::runtime_error("VEK runtime: continue outside loop");return {Flow::Continue,{}};case Stmt::Kind::Return:{ExecResult r;r.flow=Flow::Return;if(s.expression)r.value=Eval(*s.expression,env);return r;}}}return {};}
    VekValue CallFunction(const std::string&name,const std::vector<VekValue>&args){if(callDepth>=policy.maxCallDepth)throw std::runtime_error("VEK security: maximum call depth exceeded");auto it=functions.find(name);if(it==functions.end())throw std::runtime_error("VEK runtime: unknown function '"+name+"'");if(args.size()!=it->second.params.size())throw std::runtime_error("VEK runtime: function '"+name+"' expected "+std::to_string(it->second.params.size())+" args, received "+std::to_string(args.size()));++callDepth;struct Guard{std::size_t&d;~Guard(){--d;}}g{callDepth};Environment env;for(std::size_t i=0;i<args.size();++i)env[it->second.params[i]]=args[i];return ExecuteBlock(it->second.body,env).value;}

    fs::path ResolveImport(const std::string&spec,const fs::path&currentDir){
        if(spec.empty()||fs::path(spec).is_absolute()||spec.find(':')!=std::string::npos)throw std::runtime_error("VEK security: absolute/drive module paths are forbidden");
        fs::path rel=fs::path(spec);if(rel.extension().empty())rel += ".vek";
        std::vector<fs::path>roots;roots.push_back(currentDir);for(auto&r:moduleRoots)roots.push_back(r);
        for(auto&root:roots){std::error_code ec;fs::path candidate=fs::weakly_canonical(root/rel,ec);fs::path canonicalRoot=fs::weakly_canonical(root,ec);if(ec)continue;if(!PathInside(candidate,canonicalRoot))continue;if(fs::exists(candidate)&&fs::is_regular_file(candidate))return candidate;}
        throw std::runtime_error("VEK module: import not found or outside configured roots: "+spec);
    }
    std::string ExpandFile(const fs::path&path,std::unordered_set<std::string>&seen,std::size_t depth){
        if(depth>policy.maxImportDepth)throw std::runtime_error("VEK security: import depth exceeded");std::error_code ec;auto canonical=fs::weakly_canonical(path,ec);if(ec)throw std::runtime_error("VEK module: invalid path");std::string key=canonical.generic_string();if(seen.count(key))return "";if(seen.size()>=policy.maxModuleCount)throw std::runtime_error("VEK security: module count exceeded");seen.insert(key);std::ifstream in(canonical,std::ios::binary);if(!in)throw std::runtime_error("VEK module: cannot open "+key);std::ostringstream raw;raw<<in.rdbuf();if(raw.str().size()>policy.maxSourceBytes)throw std::runtime_error("VEK security: module source exceeds limit");std::istringstream lines(raw.str());std::ostringstream out;std::string line;while(std::getline(lines,line)){std::string spec;if(ParseImportLine(line,spec)){auto module=ResolveImport(spec,canonical.parent_path());out<<ExpandFile(module,seen,depth+1)<<"\n";}else out<<line<<"\n";}return out.str();}
};

VekScriptEngine::VekScriptEngine():impl(std::make_unique<Impl>()){}VekScriptEngine::~VekScriptEngine()=default;VekScriptEngine::VekScriptEngine(VekScriptEngine&&)noexcept=default;VekScriptEngine&VekScriptEngine::operator=(VekScriptEngine&&)noexcept=default;
bool VekScriptEngine::LoadFile(const std::string&path){try{std::unordered_set<std::string>seen;std::string source=impl->ExpandFile(path,seen,0);return LoadSource(source,path);}catch(const std::exception&e){impl->lastError=e.what();impl->loaded=false;return false;}}
bool VekScriptEngine::LoadSource(const std::string&source,const std::string&name){try{if(source.size()>impl->policy.maxSourceBytes*std::max<std::size_t>(1,impl->policy.maxModuleCount))throw std::runtime_error("VEK security: expanded source exceeds configured limit");Lexer l(source,impl->policy);Parser p(l.Scan(),impl->policy);auto program=p.ParseProgram();impl->functions=std::move(program.functions);impl->structs=std::move(program.structs);impl->events=std::move(program.events);impl->sourceName=name;impl->lastError.clear();impl->loaded=true;return true;}catch(const std::exception&e){impl->functions.clear();impl->structs.clear();impl->events.clear();impl->sourceName=name;impl->lastError=e.what();impl->loaded=false;return false;}}
void VekScriptEngine::Clear(){impl->functions.clear();impl->structs.clear();impl->events.clear();impl->lastError.clear();impl->sourceName.clear();impl->loaded=false;}
void VekScriptEngine::SetSecurityPolicy(const VekSecurityPolicy&p){impl->policy=p;}const VekSecurityPolicy&VekScriptEngine::GetSecurityPolicy()const{return impl->policy;}
void VekScriptEngine::SetModuleRoots(std::vector<std::string>roots){impl->moduleRoots=std::move(roots);}const std::vector<std::string>&VekScriptEngine::GetModuleRoots()const{return impl->moduleRoots;}
bool VekScriptEngine::RegisterNative(const std::string&n,NativeFunction f){
    if(impl->nativesSealed){impl->lastError="VEK security: native registry is sealed";return false;}
    if(n.empty()||n.size()>128){impl->lastError="VEK security: invalid native name";return false;}
    for(unsigned char c:n)if(!(std::isalnum(c)||c=='_')){impl->lastError="VEK security: invalid native name";return false;}
    if(impl->natives.count(n)){impl->lastError="VEK security: duplicate native registration rejected: "+n;return false;}
    impl->natives.emplace(n,std::move(f));return true;
}void VekScriptEngine::SealNativeRegistry(){impl->nativesSealed=true;}bool VekScriptEngine::NativeRegistrySealed()const{return impl->nativesSealed;}
bool VekScriptEngine::HasFunction(const std::string&n)const{return impl->functions.count(n)!=0;}bool VekScriptEngine::HasEvent(const std::string&n)const{return impl->events.count(n)!=0;}
VekValue VekScriptEngine::Call(const std::string&n,const std::vector<VekValue>&args){try{impl->lastError.clear();if(args.size()>impl->policy.maxArgumentsPerCall)throw std::runtime_error("VEK security: top-level argument limit exceeded");impl->instructionsRemaining=impl->policy.maxInstructionsPerCall;impl->nativeCallsRemaining=impl->policy.maxNativeCallsPerCall;impl->loopIterationsRemaining=impl->policy.maxLoopIterationsPerCall;impl->callDepth=0;return impl->CallFunction(n,args);}catch(const std::exception&e){impl->lastError=e.what();return {};}}
VekValue VekScriptEngine::EmitEvent(const std::string&n,const std::vector<VekValue>&args){return Call("__event_"+n,args);}bool VekScriptEngine::IsLoaded()const{return impl->loaded;}const std::string&VekScriptEngine::LastError()const{return impl->lastError;}const std::string&VekScriptEngine::SourceName()const{return impl->sourceName;}

void VekRegisterStandardLibrary(VekScriptEngine&e){
    e.RegisterNative("print",[](const std::vector<VekValue>&a){for(auto&v:a)std::cout<<v.AsString();return VekValue();});
    e.RegisterNative("println",[](const std::vector<VekValue>&a){for(std::size_t i=0;i<a.size();++i){if(i)std::cout<<" ";std::cout<<a[i].AsString();}std::cout<<"\n";return VekValue();});
    e.RegisterNative("type",[](const std::vector<VekValue>&a){if(a.empty()||a[0].IsNil())return VekValue("nil");if(a[0].IsNumber())return VekValue("number");if(a[0].IsBool())return VekValue("bool");if(a[0].IsString())return VekValue("string");if(a[0].IsArray())return VekValue("array");if(a[0].IsMap())return VekValue("map");return VekValue("unknown");});
    e.RegisterNative("len",[](const std::vector<VekValue>&a){return VekValue(a.empty()?0.0:(double)a[0].Size());});
    e.RegisterNative("number",[](const std::vector<VekValue>&a){return VekValue(a.empty()?0.0:a[0].AsNumber());});e.RegisterNative("string",[](const std::vector<VekValue>&a){return VekValue(a.empty()?"nil":a[0].AsString());});
    e.RegisterNative("json",[](const std::vector<VekValue>&a){return VekValue(a.empty()?"null":a[0].ToJson());});
    e.RegisterNative("abs",[](const std::vector<VekValue>&a){return VekValue(std::fabs(a.at(0).AsNumber()));});e.RegisterNative("floor",[](const std::vector<VekValue>&a){return VekValue(std::floor(a.at(0).AsNumber()));});e.RegisterNative("ceil",[](const std::vector<VekValue>&a){return VekValue(std::ceil(a.at(0).AsNumber()));});e.RegisterNative("round",[](const std::vector<VekValue>&a){return VekValue(std::round(a.at(0).AsNumber()));});e.RegisterNative("sqrt",[](const std::vector<VekValue>&a){return VekValue(std::sqrt(std::max(0.0,a.at(0).AsNumber())));});e.RegisterNative("pow",[](const std::vector<VekValue>&a){return VekValue(std::pow(a.at(0).AsNumber(),a.at(1).AsNumber()));});e.RegisterNative("min",[](const std::vector<VekValue>&a){return VekValue(std::min(a.at(0).AsNumber(),a.at(1).AsNumber()));});e.RegisterNative("max",[](const std::vector<VekValue>&a){return VekValue(std::max(a.at(0).AsNumber(),a.at(1).AsNumber()));});e.RegisterNative("clamp",[](const std::vector<VekValue>&a){return VekValue(std::clamp(a.at(0).AsNumber(),a.at(1).AsNumber(),a.at(2).AsNumber()));});
    e.RegisterNative("array_push",[](const std::vector<VekValue>&a){if(a.size()<2||!a[0].IsArray()||a[0].Size()>=4096)return VekValue(false);VekValue x=a[0];x.Push(a[1]);return VekValue(true);});
    e.RegisterNative("map_has",[](const std::vector<VekValue>&a){if(a.size()<2||!a[0].IsMap())return VekValue(false);auto*m=a[0].AsMap();return VekValue(m&&m->count(a[1].AsString())!=0);});
    e.RegisterNative("map_set",[](const std::vector<VekValue>&a){if(a.size()<3||!a[0].IsMap())return VekValue(false);std::string k=a[1].AsString();if(k.empty()||k.size()>1024)return VekValue(false);if(!a[0].Get(k).IsNil()||a[0].Size()<4096){VekValue x=a[0];return VekValue(x.Set(k,a[2]));}return VekValue(false);});
    e.RegisterNative("map_get",[](const std::vector<VekValue>&a){return a.size()<2?VekValue():a[0].Get(a[1].AsString());});
    e.RegisterNative("vec3",[](const std::vector<VekValue>&a){VekMap m;m["x"]=a.size()>0?a[0]:VekValue(0);m["y"]=a.size()>1?a[1]:VekValue(0);m["z"]=a.size()>2?a[2]:VekValue(0);return VekValue(std::move(m));});
    e.RegisterNative("rgba",[](const std::vector<VekValue>&a){VekMap m;m["r"]=a.size()>0?a[0]:VekValue(0);m["g"]=a.size()>1?a[1]:VekValue(0);m["b"]=a.size()>2?a[2]:VekValue(0);m["a"]=a.size()>3?a[3]:VekValue(255);return VekValue(std::move(m));});
}
