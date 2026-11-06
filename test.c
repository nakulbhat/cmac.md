#include <stdio.h>
#include <string.h>

#define TEST(x) #x

// Found at
// https://stackoverflow.com/questions/2751870/how-exactly-does-the-double-stringize-trick-work
#define _PARA(...) #__VA_ARGS__ // Macro to do the stringize
// #define PARA(...) _PARA(<p>__VA_ARGS__</p>) // Make the preprocessor expand all args first
#define BOLD(...) <b>__VA_ARGS__</b>

#define PARA(...) strcat(html_str, _PARA(<p>__VA_ARGS__</p>)) 

int main(int argc, char **argv){
    char html_str[4096] = {0};
    char *sentence = TEST(This is a sentence.);
    puts(sentence);

    PARA(This is a sentence. And here is some BOLD(BOLD, text));
    puts(html_str);


    PARA(This is a string I am adding directly to a "function");
    PARA(This is another string.);

    puts(html_str);
    return 0;
}
