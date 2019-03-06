#include "klib.h"

//#ifndef __ISA_NATIVE__
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static size_t num;
static int just;

size_t strlen(const char *s) {
    num=0;
    size_t index=0;
    while(s[index]!='\0')
    {
        num++;
        index++;
    }
    return num;
}

char *strcpy(char* dst,const char* src) {
    size_t index=0;
    while(src[index]!='\0')
    {
        dst[index]=src[index];
        index++;
    }
    dst[index]='\0';
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    size_t index;
    for(index=0;index<n&&src[index]!='\0';index++)
        dst[index]=src[index];
    for(;index<n;index++)
        dst[index]='\0';

    return dst;
}

char* strcat(char* dst, const char* src) {
    size_t dst_len=strlen(dst);
    size_t index=0;
    while(src[index]!='\0')
    {
       dst[dst_len++]=src[index++]; 
    }
    dst[dst_len]='\0';
    return dst;
}

int strcmp(const char* s1, const char* s2) {
    just=0;
    size_t index=0;
    size_t len1=strlen(s1);
    size_t len2=strlen(s2);
    size_t len;
    if(len1>len2)
        len=len1;
    else 
        len=len2;
    while(1)
    {
       if(s1[index]>s2[index])
           just=1;
       else if(s1[index]<s2[index])
           just=-1;
       else 
           just=0;

       if(just!=0)
           break;
       else if(index==len-1)
             break;
       else 
           index++;
    }
    return just;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    just=0;
    size_t index;
    for(index=0;index<n;index++)
    {
        if(s1[index]>s2[index])
            just=1;
        else if(s1[index]<s2[index])
            just=-1;
        else 
            just=0;

        if(just!=0)
            break;
    }
    return just;
}

void* memset(void* v,int c,size_t n) {
    assert(v);
    char *p=(char *)v;
    char cn=(char)c;
    for(int i=0;i<n;i++)
    {
        *p=cn;
        p++;
    }
    return v;
}

void* memcpy(void* out, const void* in, size_t n) {
     assert(out);
     assert(in);
     char *p1=(char*)in;
     char *p2=(char*)out;
     for(int i=0;i<n;i++)
     {
         *p2=*p1;
         p1++;
         p2++;
     }
    
    return out;
}

int memcmp(const void* s1, const void* s2, size_t n){
    just=0;
    if(n==0)
        return 0;
   
    unsigned char *p1=(unsigned char*)s1;
    unsigned char *p2=(unsigned char*)s2;
    for(int i=0;i<n;i++)
    {
        if(*p1>*p2)
            just=1;
        else if(*p1<*p2)
            just=-1;
        else 
            just=0;

        if(just!=0)
            break;
        else 
        {
            p1++;
            p2++;
        }
    }
    return just;
}

#endif
