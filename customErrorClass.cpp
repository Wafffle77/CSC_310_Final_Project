#include <iostream>
#include "customErrorClass.h"

using namespace std;

MyException::MyException(void){
    this->errString = "No Error Set";
}

MyException::MyException(const string &err){
    this->errString = err;
}

MyException::MyException(const char *err){
    this->errString = err;
}