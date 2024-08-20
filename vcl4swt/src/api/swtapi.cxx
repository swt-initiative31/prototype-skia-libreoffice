
/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <framework/desktop.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <i18nlangtag/languagetag.hxx>
#include <i18nlangtag/mslangid.hxx>

#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#include <sal/main.h>

#include <api/swtapi.hxx>

#include <iostream>
#include <api/app_App.h>
#include <vcl/toolkit/button.hxx>

#include <vcl/toolkit/calendar.hxx>

#include <osl/mutex.hxx>
#include <svdata.hxx>
#include <salinst.hxx>

namespace
{
class TheApplication : public Application
{
public:
    virtual int Main();
private:
    VclPtr<vcl::Window> mpWin;
};
}


VclPtr<vcl::Window> win;
VclPtr<PushButton>   button;

VclPtr<CalendarField>   calendar;

SAL_DLLPUBLIC_EXPORT int show_window(void)
{

    win = VclPtr<WorkWindow>::Create(nullptr, WB_APP );
    win->SetText(u"Minimum VCL application via library"_ustr);
    win->Show();

    return 0;

}


SAL_DLLPUBLIC_EXPORT int close_window(void)
{

	win.disposeAndClear();
	return 0;
}

	VclPtr<vcl::Window> topWin;


	SAL_DLLPUBLIC_EXPORT int set_text(void)
	{

		topWin->SetText(u"Test Change Header"_ustr);

	    return 0;

	}


	SAL_DLLPUBLIC_EXPORT int add_button(void)
	{

		 comphelper::SolarMutex *m = ImplGetSVData()->mpDefInst->GetYieldMutex();
		 m->acquire();

	    button = VclPtr<PushButton>::Create(topWin, 0);
	    button->SetText( u"Test Button"_ustr );
	    button->SetPosSizePixel( Point( 10, 300 ), Size( 120,25 ) );
	    button->Show();

	    m->release( );

	    return 0;

	}


	SAL_DLLPUBLIC_EXPORT int add_calendar(void)
	{

		 comphelper::SolarMutex *m = ImplGetSVData()->mpDefInst->GetYieldMutex();
		 m->acquire();
		 calendar = VclPtr<CalendarField>::Create(topWin, 0);
		 calendar->SetPosSizePixel( Point( 10, 300 ), Size( 120,25 ) );
		 calendar->Show();

	    m->release( );

	    return 0;

	}

int TheApplication::Main()
{
    mpWin = VclPtr<WorkWindow>::Create(nullptr, WB_APP | WB_STDWORK);
    mpWin->SetText(u"Minimum VCL application via library"_ustr);
    mpWin->Show();

    topWin = mpWin;


    Execute();
    mpWin.disposeAndClear();
    return 0;
}

SAL_DLLPUBLIC_EXPORT int show_minvcl_window(void)
{
     TheApplication aApp;

    auto xContext = cppu::defaultBootstrap_InitialComponentContext();
    css::uno::Reference<css::lang::XMultiServiceFactory> xServiceManager(
        xContext->getServiceManager(), css::uno::UNO_QUERY);
    comphelper::setProcessServiceFactory(xServiceManager);
    LanguageTag::setConfiguredSystemLanguage(MsLangId::getSystemLanguage());
    InitVCL();

    aApp.Main();

    framework::getDesktop(::comphelper::getProcessComponentContext())->terminate();
    DeInitVCL();
    comphelper::setProcessServiceFactory(nullptr);

    return 0;
}

SAL_DLLPUBLIC_EXPORT int init(void)
{
     TheApplication aApp;

    auto xContext = cppu::defaultBootstrap_InitialComponentContext();
    css::uno::Reference<css::lang::XMultiServiceFactory> xServiceManager(
        xContext->getServiceManager(), css::uno::UNO_QUERY);
    comphelper::setProcessServiceFactory(xServiceManager);
    LanguageTag::setConfiguredSystemLanguage(MsLangId::getSystemLanguage());
    InitVCL();

    return 0;

}

SAL_DLLPUBLIC_EXPORT int deinit(void)
{
     TheApplication aApp;

    auto xContext = cppu::defaultBootstrap_InitialComponentContext();
    css::uno::Reference<css::lang::XMultiServiceFactory> xServiceManager(
        xContext->getServiceManager(), css::uno::UNO_QUERY);
    comphelper::setProcessServiceFactory(xServiceManager);
    LanguageTag::setConfiguredSystemLanguage(MsLangId::getSystemLanguage());
    InitVCL();

    return 0;

}





JNIEXPORT void JNICALL Java_app_App_showMinVCLWindowUsingJNI(JNIEnv *env, jobject obj) {
    (void)env; // To avoid unused parameter warnings
    (void)obj; // To avoid unused parameter warnings
    show_minvcl_window();
    return;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
