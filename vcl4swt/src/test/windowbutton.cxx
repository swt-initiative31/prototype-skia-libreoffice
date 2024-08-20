/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sal/main.h>
#include <sal/log.hxx>

#include <cppuhelper/bootstrap.hxx>
#include <comphelper/processfactory.hxx>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <vcl/toolkit/button.hxx>
#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/graphicfilter.hxx>
#include <vcl/graph.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/extendapplicationenvironment.hxx>
#include <tools/stream.hxx>

#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>
#include <o3tl/string_view.hxx>

#include <math.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace cppu;

// Forward declaration
static void Main();

SAL_IMPLEMENT_MAIN()
{
    try
    {
        tools::extendApplicationEnvironment();

        // create the global service-manager
        Reference< XComponentContext > xContext = defaultBootstrap_InitialComponentContext();
        Reference< XMultiServiceFactory > xServiceManager( xContext->getServiceManager(), UNO_QUERY );

        if( !xServiceManager.is() )
            Application::Abort( u"Failed to bootstrap"_ustr );

        comphelper::setProcessServiceFactory( xServiceManager );

        InitVCL();
        ::Main();
        DeInitVCL();
    }
    catch (const Exception&)
    {
        TOOLS_WARN_EXCEPTION("vcl", "Fatal");
        return 1;
    }
    catch (const std::exception& e)
    {
        SAL_WARN("vcl", "Fatal: " << e.what());
        return 1;
    }

    return 0;
}

namespace {

class MyWin : public WorkWindow
{
    VclPtr<PushButton>   m_aQuitButton;

public:
                 MyWin( vcl::Window* pParent, WinBits nWinStyle );

    virtual bool Close() override;
    virtual ~MyWin() override { disposeOnce(); }
    virtual void dispose() override;

};

}

void Main()
{
    ScopedVclPtrInstance< MyWin > aMainWin( nullptr, WB_STDWORK );
    aMainWin->SetText( u"SvpClient"_ustr );
    aMainWin->Show();

    Application::Execute();
}

MyWin::MyWin( vcl::Window* pParent, WinBits nWinStyle ) :
    WorkWindow( pParent, nWinStyle ),
    m_aQuitButton(VclPtr<PushButton>::Create(this, 0))
{
    m_aQuitButton->SetPosSizePixel( Point( 10, 300 ), Size( 120,25 ) );
    m_aQuitButton->SetText( u"Quit SVP server"_ustr );
    m_aQuitButton->Show();
}

bool MyWin::Close()
{
    bool bRet = WorkWindow::Close();
    if( bRet )
        Application::Quit();
    return bRet;
}

void MyWin::dispose()
{
    m_aQuitButton.disposeAndClear();
    WorkWindow::dispose();
}



/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
