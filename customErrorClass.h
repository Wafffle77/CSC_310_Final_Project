//*********************INCLUDE GUARD*********************
#ifndef __CUSTOM_ERROR_CLASS_H__
#define __CUSTOM_ERROR_CLASS_H__
//*********************INCLUDE GUARD*********************

//*********************REQUIRED INCLUDES*********************
#include <exception>
#include <string>
//*********************REQUIRED INCLUDES*********************

using namespace std;


//*********************START CLASS MyException DEFINITION*********************
class MyException : public exception {
    public:
        MyException(void);
        MyException(const char*);
        MyException(const string&);
        virtual const string what(){
            return this->errString;
        }
    private:
        string errString;
};
//*********************END CLASS MyException DEFINITION*********************

#endif