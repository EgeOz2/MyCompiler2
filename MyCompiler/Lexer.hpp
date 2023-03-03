#include <iostream>
using namespace std;


//Lexer'in sonuç olarak kodları ayırabileceği gruplar
enum Token {
  tok_eof = -1,//End of File

  tok_fn = -2,//Fonksiyon tanimlama
  //change extern later
  tok_extern = -3,

  tok_identifier = -4,//Kullanıcının variable ve fonksiyonlara verdiği isimler
  tok_number = -5,//Float turunde olan butun sayilar
};


static string IdentifierStr;//Değişken isimleri geçici olarak buraya kaydedilr
static float NumVal;//Float sayılar geçici olarak buraya kaydedilr

//Asıl lexer fonksiyonu
//Compile'lanması gereken dosyadaki kodları teker teker karakterlerine bakarak işlevlerine göre parçalara(token) ayırarak
//Sonraki aşama olan parsing işini kolaylaştır
static int gettok(){
    static int LastChar = ' ';//Butun tek karakterler buraya geçiçi olarak kaydedilir
    
    while(isspace(LastChar)){//Kod daki bütün boşluk karakterlerini atlar
        LastChar = getchar();
    }

    if(isalpha(LastChar)){//Alpha karakterler(A-Z ve a-z) ile başlayan herşey burada sınıflara ayırılır
        IdentifierStr = LastChar;

        //Identifierlar sayı ile başlayamıyacağından dolayı yukarıda sadece alpha mı diye baktı fakat burada
        //İdentıfıerlar sayı ile devam edebileceğinden alpha numeric mi diye kontrol ediliyor
        //int 2xyz HATALI DEĞİŞKEN TANIMI
        //int xyz2 DOĞRU
        while(isalnum(LastChar = getchar())){
            IdentifierStr += LastChar;//IdentifierStr ye siradaki butun alnum karakterler ekleniyor ki kelimeler olussun
        }

        if(IdentifierStr == "fn")// Identifier fn ise bu fonksiyon tanimi olduğundan bu token tok_fn diye işaretleniyor
            return tok_fn;
        //change extern later
        if(IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;//eğer yukarıdakilerden biri değilse bu ya değişken ismidir ya da fonksiyon ismi
    }

    //Koddaki sayılar burada tanımlanır
    if(isdigit(LastChar) || LastChar == '.'){//Son karakter bir rakam veya nokta ise(float olduğundan .52)
        bool decimalFound = false;

        string NumStr;
        do{
            if(LastChar=='.'){//Hata kontrolu
                if(decimalFound==true)
                    return NULL;//Should return error
                decimalFound = true;
            } 
            NumStr += LastChar;
            LastChar = getchar();
        }while(isdigit(LastChar) || LastChar == '.');//Sayının bütün haneleri bitene kadar hane eklenir
        
        //Yukarida sayıyı string olarak kaydettiğimizden bu stringi float a çeviriliyor
        //ve NumVal global değişkenine kaydediliyor
        NumVal = strtof(NumStr.c_str(),0);
        return tok_number;//Bu karakter dizesinin sayı olduğu anlaşıldı ve token olarak kaydedildi
    }

    //Bu dilde comment yapmak # ile oluyor
    //Lexer # gördüğünde geri kalan satıra hiç bakmadan atlıyor

    /*Şu anki C++ daki gibi paragraf commentlama eklesem mi bilemedim
    belki sonra eklerim
    */
    if(LastChar == '#'){
        do{
            LastChar = getchar();
        }while(LastChar!=EOF && LastChar != '\n' && LastChar != '\r');
        
        if(LastChar != EOF)
            return gettok();
    }

    if(LastChar ==EOF)//Kod dosyasi bitti mi?
        return tok_eof;
    
    //Eğerki Yukarıdakilerden hiç biri değilse geri kalan karakter + - * / % gibi karakter olduğunda
    //Bu karakterler direk ASCII değeri ile Kaydediliyor
    int ThisChar = LastChar;
    LastChar = getchar();//Sonraki gettok() için yeni karakter alınır
    return ThisChar;
}


