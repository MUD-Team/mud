
#include <typeinfo>

#include "errors.h"

POCO_IMPLEMENT_EXCEPTION(CDoomError, Poco::Exception,
"MUD has encounted an error and must exit")