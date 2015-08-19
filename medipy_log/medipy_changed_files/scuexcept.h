/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/
 
#ifndef _05ea5b4a_5085_468c_a51e_5203a0db0e3a
#define _05ea5b4a_5085_468c_a51e_5203a0db0e3a

#ifdef _OSX
#include <string>
#endif
//Exception class to be caught by swig wrapper
class SCUException : public std::exception
{
public:
    explicit SCUException(std::string const & message);
    ~SCUException() throw() {}
    
    virtual char const * what() const throw();

private:    
    std::string message;
};

#endif //_05ea5b4a_5085_468c_a51e_5203a0db0e3a
