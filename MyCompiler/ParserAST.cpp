#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include "llvm\ADT\APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"


#include "Lexer.hpp"

using namespace llvm;
using namespace std;


// Abstract Syntax Tree
//Lexer dan çıkan tokenler parserdan geçerek diğer compiler aşamalrının işini kolaylaştırmak için
//Bazı sıralamalara göre ağaç gibi gövdeden dala daldan yaprağa yerleştirilir
class ExpressionAST {
public:
    virtual ~ExpressionAST(){}//Destructor
    virtual Value* codegen() = 0;//Virtual kullanmaktansa Visitor Pattern kullanabilirm
};

//1.5 gibi sayılar buraya kaydedilir
class NumberExpr : public ExpressionAST {
    float Val;

public:
    NumberExpr(float Val) : Val(Val){}
    Value* codegen() override;
};

//x gibi değişken isimleri
class VariableExpr : public ExpressionAST{
    string Name;

public:
    VariableExpr(const string &Name) : Name(Name){}
    Value* codegen() override;
};

// + gibi binary işlem karakterleri
class BinaryExpr : public ExpressionAST{
    char Op;
    unique_ptr<ExpressionAST> LHS,RHS;//Left/Right-Hand-Side

public:
    BinaryExpr(char op, unique_ptr<ExpressionAST> LHS, unique_ptr<ExpressionAST> RHS)
        : Op(op), LHS(move(LHS)), RHS(move(RHS)) {}
    Value* codegen() override;
};
//Fonkiyon çağırmalari için. main(x,y) gibi
class CallExpr : public ExpressionAST{
    string Callee;//Çağırılan fonksiyonun ismi
    vector<unique_ptr<ExpressionAST>> Args;//Parametreler

public:
    CallExpr(const string&Callee, vector<unique_ptr<ExpressionAST>> Args)
        : Callee(Callee), Args(move(Args)){}
    Value* codegen() override;
};

//Fonksiyon prototipleri için main(int,int); gibi
//Sonra prototipleri kaldirmayi dusunuyorum
class PrototypeAST{
    string Name;
    vector<string> Args;

public:
    PrototypeAST(const string &name, vector<string> Args)
        : Name(name), Args(move(Args)){}
    const string &getName() const {return Name;}
    Function* codegen();
};

//Fonksiyonlar için
class FunctionAST{
    unique_ptr<PrototypeAST> Proto;//?
    unique_ptr<ExpressionAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExpressionAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
    Function* codegen();
};

//Parser
//Lexer dan çıkan tokenlerin ne olduğunu, ne yaptığını ve hatalari; dilin yapısına göre analiz eder.
//Yaptığı analizleri Abstract Syntax Tree ye kaydeder
//Buradaki parser bir Recursive Descent Parser ve Operator-Precedence Parser karışımıdır
//Yukaridan aşağıya fonkisyon yinelemerline göre ve işlem(Operator) önceliğine göre çalışır


//Lexer dan sıradaki tokenı alıp bu tokenin türünü veya bir operator se ASCII değerini CurTok global değişkenine kaydeder
static int CurTok;
static int getNextToken(){
    return CurTok = gettok();
}

static 

//Hata fonksıyonu
unique_ptr<ExpressionAST> LogError(const char *Str){
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

//Prototype hata fonksiyonu
unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

//Sayı değerini AST ye kaydeder
static unique_ptr<ExpressionAST> ParseNumber(){
    auto Result = make_unique<NumberExpr>(NumVal);
    getNextToken();
    return move(Result);
}

//Parantezleri işleyen fonksıyon
static unique_ptr<ExpressionAST> ParseParenthesis(){
    getNextToken();//( atla
    auto V = ParseExpression();//Completed later?
    if(!V)
        return nullptr;

    if(CurTok != ')')
        return LogError("Parentez kapatilmamiş");
    getNextToken();
    return V;
}

//Değişken veya fonksiyon ismini IdName e kaydederek AST ye yerleştirir
//Eğer fonksiyon ise Parametreleri Args a kaydederek AST ye yerleştirir
//+ hata işleme
static unique_ptr<ExpressionAST> ParseIdentifier(){
    string IdName = IdentifierStr;//Lexer  daki ismi kaydeder

    getNextToken();//identifier atla

    if(CurTok != '(')//fonksyion degilse değişken ismini döndürr
        return make_unique<VariableExpr>(IdName);

    getNextToken();// ( atla
    vector<unique_ptr<ExpressionAST>> Args;//Parametre vektörü
    if(CurTok != ')'){//Parametreleri args a kaydeden loop
        while(true){
            if(auto Arg = ParseExpression())
                Args.push_back(move(Arg));
            else
                return nullptr;
            
            if(CurTok == ')')
                break;
            
            if(CurTok != ',')
                return LogError("Argüman listesinde ')'veya ',' bekleniyor.");
            getNextToken();
        }
    }
    getNextToken();

    return make_unique<CallExpr>(IdName,move(Args));//Fonksiyon ismini ve parametreleri AST ye kaydeder
}

//Yukarıdaki fonksiyonların şu anki karakter neyse çağırılmasını sağlayan ana Parse fonksiyonu
static std::unique_ptr<ExpressionAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("tanım beklenirken bilinmeyen ifade");
  case tok_identifier:
    return ParseIdentifier();
  case tok_number:
    return ParseNumber();
  case '(':
    return ParseParenthesis();
  }
}

//Toplama çıkarma gibi binary işlemlerini öncelik sırasına göre kaydeder
//En alltaki main fonksiyonunda 0 en düşük öncelik olup daha yüksek sayılar yüksek öncelik belirtmektedir
static map<char, int> BinaryPrecedence;

//Şu an parslanan işlemin önceliğini gösterir
static int GetTokPrecedence(){
    if(!isascii(CurTok))
        return -1; // binary degil
    
    int TokPrec = BinaryPrecedence[CurTok];
    if(TokPrec <=0) return -1;//binary degil
    return TokPrec;
}

/*
İŞLEM ÖNCELİĞİ ve OLACAK İŞLEMLER

() x++ x--
+x -x ! ++x --x
* / %
+ -
< <= > >=
== !=
ve
veya
= += -= *= /= %=
,
*/



static unique_ptr<ExpressionAST> ParseExpression(){
    auto LHS=ParsePrimary();
    if(!LHS)
        return nullptr;
    
    return ParseBinOpRHS(0,move(LHS));
}

static unique_ptr<ExpressionAST> ParseBinOpRHS(int ExprPrec, unique_ptr<ExpressionAST> LHS){
    while(1){
        int TokPrec = GetTokPrecedence();

        if(TokPrec<ExprPrec)
            return LHS;

        int BinOp=CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if(!RHS)
            return nullptr;
        
        int NextPrec = GetTokPrecedence();
        if(TokPrec<NextPrec){
            RHS = ParseBinOpRHS(TokPrec+1, move(RHS));
            if(!RHS)
                return nullptr;
        }

        LHS = make_unique<BinaryExpr>(BinOp,move(LHS),move(RHS));
    }
    
}

static unique_ptr<PrototypeAST> ParsePrototype(){
    if(CurTok != tok_identifier)
        return LogErrorP("Prototipte fonkisyon ismi bekleniyor");

    string FnName = IdentifierStr;
    getNextToken();

    if(CurTok != '(')
        return LogErrorP("Prototipte '(' lazim");

    vector<string> ArgNames;
    while(getNextToken()==tok_identifier){
        ArgNames.push_back(IdentifierStr);
    }
    if(CurTok!=')')
        return LogErrorP("Prototipte ')' lazim");
    
    getNextToken();

    return make_unique<PrototypeAST>(FnName,move(ArgNames));
}

static unique_ptr<FunctionAST> ParseDefinition(){
    getNextToken();
    auto Proto = ParsePrototype();
    if(!Proto)
        return nullptr;

    if(auto E = ParseExpression())
        return make_unique<FunctionAST>(move(Proto),move(E));
    return nullptr;
}

// Change extern later
static unique_ptr<PrototypeAST> ParseExtern(){
    getNextToken();
    return ParsePrototype();
}

static unique_ptr<FunctionAST> ParseTopLevelExpr(){
    if(auto E = ParseExpression()){
        auto Proto = make_unique<PrototypeAST>("",vector<string>());
        return make_unique<FunctionAST>(move(Proto),move(E));
    }
    return nullptr;
}


//Code generation || don't know what anything here means
static unique_ptr<LLVMContext> TheContext;
static unique_ptr<IRBuilder<>> Builder(TheContext);
static unique_ptr<Module> TheModule;
static map<string,Value*> NamedValues;

Value* LogErrorV(const char* Str) {
    LogError(Str);
    return nullptr;
}

Value* NumberExpr::codegen() {
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* VariableExpr::codegen() {
    Value* V = NamedValues[Name];
    if (!V)
        LogErrorV("Bilinmeyen değişken ismi");
    return V;
}

Value* BinaryExpr::codegen() {
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();
    if (!L || !R)
        return nullptr;
    //diğer işlemleri ekle
    switch (Op){
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return Builder->CreateFMul(L, R, "multmp");
    case'<':
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        return Builder->CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");//the context?
    default:
        return LogErrorV("geçersiz işlem ifadesi");
    }
}

Value* CallExpr::codegen() {
    Function* CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Başvurulan fonksiyon tanımlı değil");

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Girilen parametre sayısı hatalı");

    vector<Value*> ArgsV;
    for (unsigned int i = 0,  e = Args.size(); i != e; i++) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back()) {
            return nullptr;
        }
       }
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function* PrototypeAST::codegen() {
    vector<Type*> Floats(Args.size(), Type::getFloatTy(*TheContext));
    FunctionType* FT = FunctionType::get(Type::getFloatTy(*TheContext), Floats, false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    unsigned int Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function* FunctionAST::codegen() {
    Function* TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();
    if (!TheFunction)
        return nullptr;
    if (!TheFunction->empty())
        return (Function*)LogErrorV("Fonksiyon tekrar tanımlanamaz");
    
    BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto& Arg : TheFunction->args())
        NamedValues[string(Arg.getName())] = &Arg;

    if (Value* RetVal = Body->codegen()) {
        Builder->CreateRet(RetVal);
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}





//Top level parsing (Sil sonra)

static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed an extern\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_fn:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}














int main(){
    //sonra diğer işlemleri de ekle
    BinaryPrecedence['%']=40;
    BinaryPrecedence['/']=40;
    BinaryPrecedence['*']=40;
    BinaryPrecedence['-']=30;
    BinaryPrecedence['+']=30;
    BinaryPrecedence['>']=20;
    BinaryPrecedence['<']=20;
    BinaryPrecedence['=']=10;

}




