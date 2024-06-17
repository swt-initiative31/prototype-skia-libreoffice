#include <sal/config.h>

#include "api/swtapi.hxx"

#include <sal/main.h>


SAL_IMPLEMENT_MAIN()
{
    try
    {
        return show_minvcl_window();
    }
    catch (...)
    {
        return 1;
    }

    return 0;
}