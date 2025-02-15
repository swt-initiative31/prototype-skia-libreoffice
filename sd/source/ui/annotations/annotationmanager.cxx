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

#include <com/sun/star/drawing/XDrawView.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/geometry/RealPoint2D.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/document/XEventBroadcaster.hpp>
#include <com/sun/star/office/XAnnotationAccess.hpp>
#include <comphelper/lok.hxx>
#include <svx/svxids.hrc>

#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>

#include <sal/macros.h>
#include <svl/itempool.hxx>
#include <svl/intitem.hxx>
#include <unotools/localedatawrapper.hxx>
#include <unotools/useroptions.hxx>
#include <unotools/syslocale.hxx>
#include <unotools/saveopt.hxx>

#include <tools/datetime.hxx>
#include <tools/UnitConversion.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/request.hxx>
#include <sfx2/dispatch.hxx>

#include <editeng/editeng.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/outlobj.hxx>
#include <editeng/postitem.hxx>

#include <svx/postattr.hxx>

#include <annotationmanager.hxx>
#include "annotationmanagerimpl.hxx"
#include "annotationwindow.hxx"
#include <strings.hrc>

#include <Annotation.hxx>
#include <DrawDocShell.hxx>
#include <DrawViewShell.hxx>
#include <sdresid.hxx>
#include <EventMultiplexer.hxx>
#include <ViewShellBase.hxx>
#include <sdpage.hxx>
#include <drawdoc.hxx>
#include <svx/annotation/TextAPI.hxx>
#include <optsitem.hxx>
#include <sdmod.hxx>

#include <memory>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::geometry;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::ui;
using namespace ::com::sun::star::office;

namespace sd {

SfxItemPool* GetAnnotationPool()
{
    static rtl::Reference<SfxItemPool> s_pAnnotationPool;
    if( !s_pAnnotationPool )
    {
        s_pAnnotationPool = EditEngine::CreatePool();
        s_pAnnotationPool->SetUserDefaultItem(SvxFontHeightItem(423,100,EE_CHAR_FONTHEIGHT));

        vcl::Font aAppFont( Application::GetSettings().GetStyleSettings().GetAppFont() );
        s_pAnnotationPool->SetUserDefaultItem(SvxFontItem(aAppFont.GetFamilyType(),aAppFont.GetFamilyName(),u""_ustr,PITCH_DONTKNOW,RTL_TEXTENCODING_DONTKNOW,EE_CHAR_FONTINFO));
    }

    return s_pAnnotationPool.get();
}

static SfxBindings* getBindings( ViewShellBase const & rBase )
{
    if( rBase.GetMainViewShell() && rBase.GetMainViewShell()->GetViewFrame() )
        return &rBase.GetMainViewShell()->GetViewFrame()->GetBindings();

    return nullptr;
}

static SfxDispatcher* getDispatcher( ViewShellBase const & rBase )
{
    if( rBase.GetMainViewShell() && rBase.GetMainViewShell()->GetViewFrame() )
        return rBase.GetMainViewShell()->GetViewFrame()->GetDispatcher();

    return nullptr;
}

css::util::DateTime getCurrentDateTime()
{
    DateTime aCurrentDate( DateTime::SYSTEM );
    return css::util::DateTime( 0, aCurrentDate.GetSec(),
            aCurrentDate.GetMin(), aCurrentDate.GetHour(),
            aCurrentDate.GetDay(), aCurrentDate.GetMonth(),
            aCurrentDate.GetYear(), false );
}

OUString getAnnotationDateTimeString( const Reference< XAnnotation >& xAnnotation )
{
    OUString sRet;
    if( xAnnotation.is() )
    {
        const SvtSysLocale aSysLocale;
        const LocaleDataWrapper& rLocalData = aSysLocale.GetLocaleData();

        css::util::DateTime aDateTime( xAnnotation->getDateTime() );

        Date aSysDate( Date::SYSTEM );
        Date aDate( aDateTime.Day, aDateTime.Month, aDateTime.Year );
        if (aDate==aSysDate)
            sRet = SdResId(STR_ANNOTATION_TODAY);
        else if (aDate == (aSysDate-1))
            sRet = SdResId(STR_ANNOTATION_YESTERDAY);
        else if (aDate.IsValidAndGregorian() )
            sRet = rLocalData.getDate(aDate);

        ::tools::Time aTime( aDateTime );
        if(aTime.GetTime() != 0)
            sRet += " "  + rLocalData.getTime( aTime,false );
    }
    return sRet;
}

AnnotationManagerImpl::AnnotationManagerImpl( ViewShellBase& rViewShellBase )
: mrBase( rViewShellBase )
, mpDoc( rViewShellBase.GetDocument() )
, mbShowAnnotations( true )
, mnUpdateTagsEvent( nullptr )
{
    SdOptions* pOptions = SD_MOD()->GetSdOptions(mpDoc->GetDocumentType());
    if( pOptions )
        mbShowAnnotations = pOptions->IsShowComments();
}

void AnnotationManagerImpl::init()
{
    // get current controller and initialize listeners
    try
    {
        addListener();
        mxView.set(mrBase.GetController(), UNO_QUERY);
    }
    catch( Exception& )
    {
        TOOLS_WARN_EXCEPTION( "sd", "sd::AnnotationManagerImpl::AnnotationManagerImpl()" );
    }

    try
    {
        Reference<XEventBroadcaster> xModel (mrBase.GetDocShell()->GetModel(), UNO_QUERY_THROW );
        Reference<XEventListener> xListener( this );
        xModel->addEventListener( xListener );
    }
    catch( Exception& )
    {
    }
}

// WeakComponentImplHelper
void AnnotationManagerImpl::disposing (std::unique_lock<std::mutex>&)
{
    try
    {
        Reference<XEventBroadcaster> xModel (mrBase.GetDocShell()->GetModel(), UNO_QUERY_THROW );
        Reference<XEventListener> xListener( this );
        xModel->removeEventListener( xListener );
    }
    catch( Exception& )
    {
    }

    removeListener();
    DisposeTags();

    if( mnUpdateTagsEvent )
    {
        Application::RemoveUserEvent( mnUpdateTagsEvent );
        mnUpdateTagsEvent = nullptr;
    }

    mxView.clear();
    mxCurrentPage.clear();
}

// XEventListener
void SAL_CALL AnnotationManagerImpl::notifyEvent( const css::document::EventObject& aEvent )
{
    if( !(aEvent.EventName == "OnAnnotationInserted" || aEvent.EventName == "OnAnnotationRemoved" || aEvent.EventName == "OnAnnotationChanged") )
        return;

    // AnnotationInsertion and modification is not handled here because when
    // a new annotation is inserted, it consists of OnAnnotationInserted
    // followed by a chain of OnAnnotationChanged (called for setting each
    // of the annotation attributes - author, text etc.). This is not what a
    // LOK client wants. So only handle removal here as annotation removal
    // consists of only one event - 'OnAnnotationRemoved'
    if ( aEvent.EventName == "OnAnnotationRemoved" )
    {
        Reference< XAnnotation > xAnnotation( aEvent.Source, uno::UNO_QUERY );
        if ( auto pAnnotation = dynamic_cast<sd::Annotation*>(xAnnotation.get()) )
        {
            LOKCommentNotify(sdr::annotation::CommentNotificationType::Remove, &mrBase, *pAnnotation);
        }
    }

    UpdateTags();
}

void SAL_CALL AnnotationManagerImpl::disposing( const css::lang::EventObject& /*Source*/ )
{
}

rtl::Reference<sdr::annotation::Annotation> AnnotationManagerImpl::GetAnnotationById(sal_uInt32 nAnnotationId)
{
    SdPage* pPage = nullptr;
    do
    {
        pPage = GetNextPage(pPage, true);
        if( pPage && !pPage->getAnnotations().empty() )
        {
            sdr::annotation::AnnotationVector aAnnotations(pPage->getAnnotations());
            auto iterator = std::find_if(aAnnotations.begin(), aAnnotations.end(),
                [nAnnotationId](rtl::Reference<sdr::annotation::Annotation> const& xAnnotation)
                {
                        return xAnnotation->GetId() == nAnnotationId;
                });
            if (iterator != aAnnotations.end())
                return *iterator;
        }
    } while(pPage);

    rtl::Reference<sdr::annotation::Annotation> xAnnotationEmpty;
    return xAnnotationEmpty;
}

void AnnotationManagerImpl::ShowAnnotations( bool bShow )
{
    // enforce show annotations if a new annotation is inserted
    if( mbShowAnnotations != bShow )
    {
        mbShowAnnotations = bShow;

        SdOptions* pOptions = SD_MOD()->GetSdOptions(mpDoc->GetDocumentType());
        if( pOptions )
            pOptions->SetShowComments( mbShowAnnotations );

        UpdateTags();
    }
}

void AnnotationManagerImpl::ExecuteAnnotation(SfxRequest const & rReq )
{
    switch( rReq.GetSlot() )
    {
    case SID_INSERT_POSTIT:
        ExecuteInsertAnnotation( rReq );
        break;
    case SID_DELETE_POSTIT:
    case SID_DELETEALL_POSTIT:
    case SID_DELETEALLBYAUTHOR_POSTIT:
        ExecuteDeleteAnnotation( rReq );
        break;
    case SID_EDIT_POSTIT:
        ExecuteEditAnnotation( rReq );
        break;
    case SID_PREVIOUS_POSTIT:
    case SID_NEXT_POSTIT:
        SelectNextAnnotation( rReq.GetSlot() == SID_NEXT_POSTIT );
        break;
    case SID_REPLYTO_POSTIT:
        ExecuteReplyToAnnotation( rReq );
        break;
    case SID_TOGGLE_NOTES:
        ShowAnnotations( !mbShowAnnotations );
        break;
    }
}

void AnnotationManagerImpl::ExecuteInsertAnnotation(SfxRequest const & rReq)
{
    if (!comphelper::LibreOfficeKit::isActive() || comphelper::LibreOfficeKit::isTiledAnnotations())
        ShowAnnotations(true);

    const SfxItemSet* pArgs = rReq.GetArgs();
    OUString sText;
    if (pArgs)
    {
        if (const SfxStringItem* pPoolItem = pArgs->GetItemIfSet(SID_ATTR_POSTIT_TEXT))
        {
            sText = pPoolItem->GetValue();
        }
    }

    InsertAnnotation(sText);
}

void AnnotationManagerImpl::ExecuteDeleteAnnotation(SfxRequest const & rReq)
{
    ShowAnnotations( true );

    const SfxItemSet* pArgs = rReq.GetArgs();

    switch( rReq.GetSlot() )
    {
    case SID_DELETEALL_POSTIT:
        DeleteAllAnnotations();
        break;
    case SID_DELETEALLBYAUTHOR_POSTIT:
        if( pArgs )
        {
            const SfxPoolItem*  pPoolItem = nullptr;
            if( SfxItemState::SET == pArgs->GetItemState( SID_DELETEALLBYAUTHOR_POSTIT, true, &pPoolItem ) )
            {
                OUString sAuthor( static_cast<const SfxStringItem*>( pPoolItem )->GetValue() );
                DeleteAnnotationsByAuthor( sAuthor );
            }
        }
        break;
    case SID_DELETE_POSTIT:
        {
            rtl::Reference<sdr::annotation::Annotation> xAnnotation;
            sal_uInt32 nId = 0;
            if( pArgs )
            {
                const SfxPoolItem*  pPoolItem = nullptr;
                if( SfxItemState::SET == pArgs->GetItemState( SID_DELETE_POSTIT, true, &pPoolItem ) )
                {
                    uno::Reference<XAnnotation> xTmpAnnotation;
                    if (static_cast<const SfxUnoAnyItem*>(pPoolItem)->GetValue() >>= xTmpAnnotation)
                    {
                        xAnnotation = dynamic_cast<sdr::annotation::Annotation*>(xTmpAnnotation.get());
                        assert(bool(xAnnotation) == bool(xTmpAnnotation) && "must be of concrete type sd::Annotation");
                    }
                }
                if( SfxItemState::SET == pArgs->GetItemState( SID_ATTR_POSTIT_ID, true, &pPoolItem ) )
                    nId = static_cast<const SvxPostItIdItem*>(pPoolItem)->GetValue().toUInt32();
            }

            if (nId != 0)
                xAnnotation = GetAnnotationById(nId);
            else if( !xAnnotation.is() )
                GetSelectedAnnotation(xAnnotation);

            DeleteAnnotation(xAnnotation);
        }
        break;
    }

    UpdateTags();
}

void AnnotationManagerImpl::ExecuteEditAnnotation(SfxRequest const & rReq)
{
    const SfxItemSet* pArgs = rReq.GetArgs();
    rtl::Reference<sdr::annotation::Annotation> xAnnotation;
    OUString sText;
    sal_Int32 nPositionX = -1;
    sal_Int32 nPositionY = -1;

    if (!pArgs)
        return;

    if (mpDoc->IsUndoEnabled())
        mpDoc->BegUndo(SdResId(STR_ANNOTATION_UNDO_EDIT));

    if (const SvxPostItIdItem* pPoolItem = pArgs->GetItemIfSet(SID_ATTR_POSTIT_ID))
    {
        sal_uInt32 nId = pPoolItem->GetValue().toUInt32();
        xAnnotation = GetAnnotationById(nId);
    }
    if (const SfxStringItem* pPoolItem = pArgs->GetItemIfSet(SID_ATTR_POSTIT_TEXT))
        sText = pPoolItem->GetValue();

    if (const SfxInt32Item* pPoolItem = pArgs->GetItemIfSet(SID_ATTR_POSTIT_POSITION_X))
        nPositionX = pPoolItem->GetValue();

    if (const SfxInt32Item* pPoolItem = pArgs->GetItemIfSet(SID_ATTR_POSTIT_POSITION_Y))
        nPositionY = pPoolItem->GetValue();

    if (xAnnotation.is())
    {
        auto pSdAnnotation = static_cast<sd::Annotation*>(xAnnotation.get());
        pSdAnnotation->createChangeUndo();

        if (nPositionX >= 0 && nPositionY >= 0)
        {
            double fX = convertTwipToMm100(nPositionX) / 100.0;
            double fY = convertTwipToMm100(nPositionY) / 100.0;
            xAnnotation->setPosition({fX, fY});
        }

        if (!sText.isEmpty())
        {
            // TODO: Not allow other authors to change others' comments ?
            Reference<XText> xText(xAnnotation->getTextRange());
            xText->setString(sText);
        }

        LOKCommentNotifyAll(sdr::annotation::CommentNotificationType::Modify, *xAnnotation);
    }

    if (mpDoc->IsUndoEnabled())
        mpDoc->EndUndo();

    UpdateTags(true);
}

void AnnotationManagerImpl::InsertAnnotation(const OUString& rText)
{
    SdPage* pPage = GetCurrentPage();
    if( !pPage )
        return;

    if( mpDoc->IsUndoEnabled() )
        mpDoc->BegUndo( SdResId( STR_ANNOTATION_UNDO_INSERT ) );

    // find free space for new annotation
    int y = 0, x = 0;

    sdr::annotation::AnnotationVector aAnnotations(pPage->getAnnotations());
    if( !aAnnotations.empty() )
    {
        const int page_width = pPage->GetSize().Width();
        const int width = 1000;
        const int height = 800;
        ::tools::Rectangle aTagRect;

        while( true )
        {
            ::tools::Rectangle aNewRect( x, y, x + width - 1, y + height - 1 );
            bool bFree = true;

            for( const auto& rxAnnotation : aAnnotations )
            {
                RealPoint2D aPoint( rxAnnotation->getPosition() );
                aTagRect.SetLeft( sal::static_int_cast< ::tools::Long >( aPoint.X * 100.0 ) );
                aTagRect.SetTop( sal::static_int_cast< ::tools::Long >( aPoint.Y * 100.0 ) );
                aTagRect.SetRight( aTagRect.Left() + width - 1 );
                aTagRect.SetBottom( aTagRect.Top() + height - 1 );

                if( aNewRect.Overlaps( aTagRect ) )
                {
                    bFree = false;
                    break;
                }
            }

            if( !bFree )
            {
                x += width;
                if( x > page_width )
                {
                    x = 0;
                    y += height;
                }
            }
            else
            {
                break;
            }
        }
    }

    rtl::Reference<sdr::annotation::Annotation> xAnnotation = pPage->createAnnotation();

    OUString sAuthor;
    if (comphelper::LibreOfficeKit::isActive())
        sAuthor = mrBase.GetMainViewShell()->GetView()->GetAuthor();
    else
    {
        SvtUserOptions aUserOptions;
        sAuthor = aUserOptions.GetFullName();
        xAnnotation->setInitials( aUserOptions.GetID() );
    }

    if (!rText.isEmpty())
    {
        Reference<XText> xText(xAnnotation->getTextRange());
        xText->setString(rText);
    }

    // set current author to new annotation
    xAnnotation->setAuthor( sAuthor );
    // set current time to new annotation
    xAnnotation->setDateTime( getCurrentDateTime() );

    // set position
    RealPoint2D aPos( static_cast<double>(x) / 100.0, static_cast<double>(y) / 100.0 );
    xAnnotation->setPosition( aPos );

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();

    // Tell our LOK clients about new comment added
    LOKCommentNotifyAll(sdr::annotation::CommentNotificationType::Add, *xAnnotation);

    UpdateTags(true);
    SelectAnnotation( xAnnotation, true );
}

void AnnotationManagerImpl::ExecuteReplyToAnnotation( SfxRequest const & rReq )
{
    rtl::Reference< sdr::annotation::Annotation> xAnnotation;
    const SfxItemSet* pArgs = rReq.GetArgs();
    OUString sReplyText;
    if( pArgs )
    {
        const SfxPoolItem*  pPoolItem = nullptr;
        if( SfxItemState::SET == pArgs->GetItemState( SID_ATTR_POSTIT_ID, true, &pPoolItem ) )
        {
            sal_uInt32 nReplyId = 0; // Id of the comment to reply to
            nReplyId = static_cast<const SvxPostItIdItem*>(pPoolItem)->GetValue().toUInt32();
            xAnnotation = GetAnnotationById(nReplyId);
        }
        else if( SfxItemState::SET == pArgs->GetItemState( rReq.GetSlot(), true, &pPoolItem ) )
        {
            uno::Reference<XAnnotation> xTmpAnnotation;
            if (static_cast<const SfxUnoAnyItem*>(pPoolItem)->GetValue() >>= xTmpAnnotation)
            {
                xAnnotation = dynamic_cast<Annotation*>(xTmpAnnotation.get());
                assert(bool(xAnnotation) == bool(xTmpAnnotation) && "must be of concrete type sd::Annotation");
            }
        }

        if( SfxItemState::SET == pArgs->GetItemState( SID_ATTR_POSTIT_TEXT, true, &pPoolItem ) )
            sReplyText = static_cast<const SvxPostItTextItem*>( pPoolItem )->GetValue();
    }

    auto* pTextApi = getTextApiObject( xAnnotation );
    if( !pTextApi )
        return;

    if (mpDoc->IsUndoEnabled())
        mpDoc->BegUndo(SdResId(STR_ANNOTATION_REPLY));

    if (xAnnotation)
    {
        auto pSdAnnotation = static_cast<sd::Annotation*>(xAnnotation.get());
        pSdAnnotation->createChangeUndo();
    }

    ::Outliner aOutliner( GetAnnotationPool(),OutlinerMode::TextObject );

    SdDrawDocument::SetCalcFieldValueHdl( &aOutliner );
    aOutliner.SetUpdateLayout( true );

    OUString aStr(SdResId(STR_ANNOTATION_REPLY));
    OUString sAuthor( xAnnotation->getAuthor() );
    if( sAuthor.isEmpty() )
        sAuthor = SdResId( STR_ANNOTATION_NOAUTHOR );

    aStr = aStr.replaceFirst("%1", sAuthor) +
        " (" + getAnnotationDateTimeString( xAnnotation ) + "): \"";

    OUString sQuote( pTextApi->GetText() );

    if( sQuote.isEmpty() )
        sQuote = "...";
    aStr += sQuote + "\"\n";

    for( sal_Int32 nIdx = 0; nIdx >= 0; )
        aOutliner.Insert( aStr.getToken( 0, '\n', nIdx ), EE_PARA_APPEND, -1 );

    if( aOutliner.GetParagraphCount() > 1 )
    {
        SfxItemSet aAnswerSet( aOutliner.GetEmptyItemSet() );
        aAnswerSet.Put(SvxPostureItem(ITALIC_NORMAL,EE_CHAR_ITALIC));

        ESelection aSel;
        aSel.nEndPara = aOutliner.GetParagraphCount()-2;
        aSel.nEndPos = aOutliner.GetText( aOutliner.GetParagraph( aSel.nEndPara ) ).getLength();

        aOutliner.QuickSetAttribs( aAnswerSet, aSel );
    }

    if (!sReplyText.isEmpty())
        aOutliner.Insert(sReplyText);

    std::optional< OutlinerParaObject > pOPO( aOutliner.CreateParaObject() );
    pTextApi->SetText(*pOPO);

    OUString sReplyAuthor;
    if (comphelper::LibreOfficeKit::isActive())
        sReplyAuthor = mrBase.GetMainViewShell()->GetView()->GetAuthor();
    else
    {
        SvtUserOptions aUserOptions;
        sReplyAuthor = aUserOptions.GetFullName();
        xAnnotation->setInitials( aUserOptions.GetID() );
    }

    xAnnotation->setAuthor( sReplyAuthor );
    // set current time to reply
    xAnnotation->setDateTime( getCurrentDateTime() );

    // Tell our LOK clients about this (comment modification)
    LOKCommentNotifyAll(sdr::annotation::CommentNotificationType::Modify, *xAnnotation);

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();

    UpdateTags(true);
    SelectAnnotation( xAnnotation, true );
}

void AnnotationManagerImpl::DeleteAnnotation(rtl::Reference<sdr::annotation::Annotation> const& xAnnotation )
{
    SdPage* pPage = GetCurrentPage();

    if( xAnnotation.is() && pPage )
    {
        if( mpDoc->IsUndoEnabled() )
            mpDoc->BegUndo( SdResId( STR_ANNOTATION_UNDO_DELETE ) );

        pPage->removeAnnotation( xAnnotation );

        if( mpDoc->IsUndoEnabled() )
            mpDoc->EndUndo();

        UpdateTags();
    }
}

void AnnotationManagerImpl::DeleteAnnotationsByAuthor( std::u16string_view sAuthor )
{
    if( mpDoc->IsUndoEnabled() )
        mpDoc->BegUndo( SdResId( STR_ANNOTATION_UNDO_DELETE ) );

    SdPage* pPage = nullptr;
    do
    {
        pPage = GetNextPage( pPage, true );

        if( pPage )
        {
            for (auto const& xAnnotation : pPage->getAnnotations())
            {
                if( xAnnotation->getAuthor() == sAuthor )
                {
                    if( mxSelectedAnnotation == xAnnotation )
                        mxSelectedAnnotation.clear();
                    pPage->removeAnnotation( xAnnotation );
                }
            }
        }
    } while( pPage );

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();
}

void AnnotationManagerImpl::DeleteAllAnnotations()
{
    if( mpDoc->IsUndoEnabled() )
        mpDoc->BegUndo( SdResId( STR_ANNOTATION_UNDO_DELETE ) );

    SdPage* pPage = nullptr;
    do
    {
        pPage = GetNextPage( pPage, true );

        if( pPage && !pPage->getAnnotations().empty() )
        {

            for( const auto& rxAnnotation : pPage->getAnnotations())
            {
                pPage->removeAnnotation( rxAnnotation );
            }
        }
    }
    while( pPage );

    mxSelectedAnnotation.clear();

    if( mpDoc->IsUndoEnabled() )
        mpDoc->EndUndo();
}

void AnnotationManagerImpl::GetAnnotationState(SfxItemSet& rSet)
{
    SdPage* pCurrentPage = GetCurrentPage();

    const bool bReadOnly = mrBase.GetDocShell()->IsReadOnly();
    const bool bWrongPageKind = (pCurrentPage == nullptr) || (pCurrentPage->GetPageKind() != PageKind::Standard);

    const SvtSaveOptions::ODFSaneDefaultVersion nCurrentODFVersion( GetODFSaneDefaultVersion() );

    if (bReadOnly || bWrongPageKind || (nCurrentODFVersion <= SvtSaveOptions::ODFSVER_012))
        rSet.DisableItem( SID_INSERT_POSTIT );

    rSet.Put(SfxBoolItem(SID_TOGGLE_NOTES, mbShowAnnotations));

    rtl::Reference<sdr::annotation::Annotation> xAnnotation;
    GetSelectedAnnotation(xAnnotation);

    // Don't disable these slot in case of LOK, as postit doesn't need to
    // selected before doing an operation on it in LOK
    if( (!xAnnotation.is() && !comphelper::LibreOfficeKit::isActive()) || bReadOnly )
    {
        rSet.DisableItem( SID_DELETE_POSTIT );
        rSet.DisableItem( SID_EDIT_POSTIT );
    }

    SdPage* pPage = nullptr;

    bool bHasAnnotations = false;
    do
    {
        pPage = GetNextPage( pPage, true );

        if( pPage && !pPage->getAnnotations().empty() )
            bHasAnnotations = true;
    }
    while( pPage && !bHasAnnotations );

    if( !bHasAnnotations || bReadOnly )
    {
        rSet.DisableItem( SID_DELETEALL_POSTIT );
    }

    if( bWrongPageKind || !bHasAnnotations )
    {
        rSet.DisableItem( SID_PREVIOUS_POSTIT );
        rSet.DisableItem( SID_NEXT_POSTIT );
    }
}

void AnnotationManagerImpl::SelectNextAnnotation(bool bForward)
{
    ShowAnnotations( true );

    rtl::Reference<sdr::annotation::Annotation> xCurrent;
    GetSelectedAnnotation(xCurrent);
    SdPage* pPage = GetCurrentPage();
    if( !pPage )
        return;

    sdr::annotation::AnnotationVector const& aAnnotations = pPage->getAnnotations();

    if( bForward )
    {
        if( xCurrent.is() )
        {
            auto iter = std::find(aAnnotations.begin(), aAnnotations.end(), xCurrent);
            if (iter != aAnnotations.end())
            {
                ++iter;
                if( iter != aAnnotations.end() )
                {
                    SelectAnnotation( *iter );
                    return;
                }
            }
        }
        else if( !aAnnotations.empty() )
        {
            SelectAnnotation( *(aAnnotations.begin()) );
            return;
        }
    }
    else
    {
        if( xCurrent.is() )
        {
            auto iter = std::find(aAnnotations.begin(), aAnnotations.end(), xCurrent);
            if (iter != aAnnotations.end() && iter != aAnnotations.begin())
            {
                --iter;
                SelectAnnotation( *iter );
                return;
            }
        }
        else if( !aAnnotations.empty() )
        {
            auto iterator = aAnnotations.end();
            iterator--;
            SelectAnnotation(*iterator);
            return;
        }
    }

    mxSelectedAnnotation.clear();
    do
    {
        do
        {
            pPage = GetNextPage( pPage, bForward );

            if( pPage && !pPage->getAnnotations().empty() )
            {
                // switch to next/previous slide with annotations
                std::shared_ptr<DrawViewShell> pDrawViewShell(std::dynamic_pointer_cast<DrawViewShell>(mrBase.GetMainViewShell()));
                if (pDrawViewShell != nullptr)
                {
                    pDrawViewShell->ChangeEditMode(pPage->IsMasterPage() ? EditMode::MasterPage : EditMode::Page, false);
                    pDrawViewShell->SwitchPage((pPage->GetPageNum() - 1) >> 1);

                    SfxDispatcher* pDispatcher = getDispatcher( mrBase );
                    if( pDispatcher )
                        pDispatcher->Execute( bForward ? SID_NEXT_POSTIT : SID_PREVIOUS_POSTIT );

                    return;
                }
            }
        }
        while( pPage );

        // The question text depends on the search direction.
        bool bImpress = mpDoc->GetDocumentType() == DocumentType::Impress;
        TranslateId pStringId;
        if(bForward)
            pStringId = bImpress ? STR_ANNOTATION_WRAP_FORWARD : STR_ANNOTATION_WRAP_FORWARD_DRAW;
        else
            pStringId = bImpress ? STR_ANNOTATION_WRAP_BACKWARD : STR_ANNOTATION_WRAP_BACKWARD_DRAW;

        // Pop up question box that asks the user whether to wrap around.
        // The dialog is made modal with respect to the whole application.
        std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(nullptr,
                                                       VclMessageType::Question, VclButtonsType::YesNo,
                                                       SdResId(pStringId)));
        xQueryBox->set_default_response(RET_YES);
        if (xQueryBox->run() != RET_YES)
            break;
    }
    while( true );
}

void AnnotationManagerImpl::onTagSelected( AnnotationTag const & rTag )
{
    mxSelectedAnnotation = rTag.GetAnnotation();
    invalidateSlots();
}

void AnnotationManagerImpl::onTagDeselected( AnnotationTag const & rTag )
{
    if (rTag.GetAnnotation() == mxSelectedAnnotation)
    {
        mxSelectedAnnotation.clear();
        invalidateSlots();
    }
}

void AnnotationManagerImpl::SelectAnnotation(rtl::Reference<sdr::annotation::Annotation> const& xAnnotation, bool bEdit)
{
    mxSelectedAnnotation = xAnnotation;

    auto iter = std::find_if(maTagVector.begin(), maTagVector.end(),
        [&xAnnotation](const rtl::Reference<AnnotationTag>& rxTag) { return rxTag->GetAnnotation() == xAnnotation; });
    if (iter != maTagVector.end())
    {
        SmartTagReference xTag( *iter );
        mrBase.GetMainViewShell()->GetView()->getSmartTags().select( xTag );
        (*iter)->OpenPopup( bEdit );
    }
}

void AnnotationManagerImpl::GetSelectedAnnotation( rtl::Reference<sdr::annotation::Annotation>& xAnnotation )
{
    xAnnotation = mxSelectedAnnotation;
}

void AnnotationManagerImpl::invalidateSlots()
{
    SfxBindings* pBindings = getBindings( mrBase );
    if( pBindings )
    {
        pBindings->Invalidate( SID_INSERT_POSTIT );
        pBindings->Invalidate( SID_DELETE_POSTIT );
        pBindings->Invalidate( SID_DELETEALL_POSTIT );
        pBindings->Invalidate( SID_PREVIOUS_POSTIT );
        pBindings->Invalidate( SID_NEXT_POSTIT );
        pBindings->Invalidate( SID_UNDO );
        pBindings->Invalidate( SID_REDO );
    }
}

void AnnotationManagerImpl::onSelectionChanged()
{
    if( !(mxView.is() && mrBase.GetDrawView()) )
        return;

    try
    {
        rtl::Reference< SdPage > xPage = mrBase.GetMainViewShell()->getCurrentPage();

        if( xPage != mxCurrentPage )
        {
            mxCurrentPage = xPage;

            UpdateTags(true);
        }
    }
    catch( Exception& )
    {
        TOOLS_WARN_EXCEPTION( "sd", "sd::AnnotationManagerImpl::onSelectionChanged()" );
    }
}

void AnnotationManagerImpl::UpdateTags( bool bSynchron )
{
    if( bSynchron )
    {
        if( mnUpdateTagsEvent )
            Application::RemoveUserEvent( mnUpdateTagsEvent );

        UpdateTagsHdl(nullptr);
    }
    else
    {
        if( !mnUpdateTagsEvent && mxView.is() )
            mnUpdateTagsEvent = Application::PostUserEvent( LINK( this, AnnotationManagerImpl, UpdateTagsHdl ) );
    }
}

IMPL_LINK_NOARG(AnnotationManagerImpl, UpdateTagsHdl, void*, void)
{
    mnUpdateTagsEvent  = nullptr;
    DisposeTags();

    if( mbShowAnnotations )
        CreateTags();

    if( mrBase.GetDrawView() )
        static_cast< ::sd::View* >( mrBase.GetDrawView() )->updateHandles();

    invalidateSlots();
}

void AnnotationManagerImpl::CreateTags()
{
    if( !(mxCurrentPage.is() && mpDoc) )
        return;

    auto xViewShell = mrBase.GetMainViewShell();
    if (!xViewShell)
        return;

    try
    {
        int nIndex = 1;
        maFont = Application::GetSettings().GetStyleSettings().GetAppFont();

        rtl::Reference< AnnotationTag > xSelectedTag;

        for (rtl::Reference<sdr::annotation::Annotation> const& xAnnotation : mxCurrentPage->getAnnotations())
        {
            Color aColor( GetColorLight( mpDoc->GetAnnotationAuthorIndex( xAnnotation->getAuthor() ) ) );
            rtl::Reference< AnnotationTag > xTag( new AnnotationTag( *this, *xViewShell->GetView(), xAnnotation, aColor, nIndex++, maFont ) );
            maTagVector.push_back(xTag);

            if (xAnnotation == mxSelectedAnnotation)
            {
                xSelectedTag = xTag;
            }
        }

        if( xSelectedTag.is() )
        {
            SmartTagReference xTag( xSelectedTag );
            mrBase.GetMainViewShell()->GetView()->getSmartTags().select( xTag );
        }
        else
        {
            // no tag, no selection!
            mxSelectedAnnotation.clear();
        }
    }
    catch( Exception& )
    {
        TOOLS_WARN_EXCEPTION( "sd", "sd::AnnotationManagerImpl::onSelectionChanged()" );
    }
}

void AnnotationManagerImpl::DisposeTags()
{
    for (auto& rxTag : maTagVector)
    {
        rxTag->Dispose();
    }

    maTagVector.clear();
}

void AnnotationManagerImpl::addListener()
{
    Link<tools::EventMultiplexerEvent&,void> aLink( LINK(this,AnnotationManagerImpl,EventMultiplexerListener) );
    mrBase.GetEventMultiplexer()->AddEventListener(aLink);
}

void AnnotationManagerImpl::removeListener()
{
    Link<tools::EventMultiplexerEvent&,void> aLink( LINK(this,AnnotationManagerImpl,EventMultiplexerListener) );
    mrBase.GetEventMultiplexer()->RemoveEventListener( aLink );
}

IMPL_LINK(AnnotationManagerImpl,EventMultiplexerListener,
    tools::EventMultiplexerEvent&, rEvent, void)
{
    switch (rEvent.meEventId)
    {
        case EventMultiplexerEventId::CurrentPageChanged:
        case EventMultiplexerEventId::EditViewSelection:
            onSelectionChanged();
            break;

        case EventMultiplexerEventId::MainViewRemoved:
            mxView.clear();
            onSelectionChanged();
            break;

        case EventMultiplexerEventId::MainViewAdded:
            mxView.set( mrBase.GetController(), UNO_QUERY );
            onSelectionChanged();
            break;

        default: break;
    }
}

void AnnotationManagerImpl::ExecuteAnnotationTagContextMenu(const rtl::Reference<sdr::annotation::Annotation>& xAnnotation, weld::Widget* pParent, const ::tools::Rectangle& rContextRect)
{
    SfxDispatcher* pDispatcher( getDispatcher( mrBase ) );
    if( !pDispatcher )
        return;

    const bool bReadOnly = mrBase.GetDocShell()->IsReadOnly();

    if (bReadOnly)
        return;

    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(pParent, u"modules/simpress/ui/annotationtagmenu.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu(xBuilder->weld_menu(u"menu"_ustr));

    SvtUserOptions aUserOptions;
    OUString sCurrentAuthor( aUserOptions.GetFullName() );
    OUString sAuthor( xAnnotation->getAuthor() );

    OUString aStr(xMenu->get_label(u".uno:DeleteAllAnnotationByAuthor"_ustr));
    OUString aReplace( sAuthor );
    if( aReplace.isEmpty() )
        aReplace = SdResId( STR_ANNOTATION_NOAUTHOR );
    aStr = aStr.replaceFirst("%1", aReplace);
    xMenu->set_label(u".uno:DeleteAllAnnotationByAuthor"_ustr, aStr);

    bool bShowReply = sAuthor != sCurrentAuthor;
    xMenu->set_visible(u".uno:ReplyToAnnotation"_ustr, bShowReply);
    xMenu->set_visible(u"separator"_ustr, bShowReply);
    xMenu->set_visible(u".uno:DeleteAnnotation"_ustr, xAnnotation.is());

    auto sId = xMenu->popup_at_rect(pParent, rContextRect);

    if (sId == ".uno:ReplyToAnnotation")
    {
        const SfxUnoAnyItem aItem( SID_REPLYTO_POSTIT, Any( css::uno::Reference<XInterface>(static_cast<cppu::OWeakObject*>(xAnnotation.get())) ) );
        pDispatcher->ExecuteList(SID_REPLYTO_POSTIT,
                SfxCallMode::ASYNCHRON, { &aItem });
    }
    else if (sId == ".uno:DeleteAnnotation")
    {
        const SfxUnoAnyItem aItem( SID_REPLYTO_POSTIT, Any( css::uno::Reference<XInterface>(static_cast<cppu::OWeakObject*>(xAnnotation.get())) ) );
        pDispatcher->ExecuteList(SID_DELETE_POSTIT, SfxCallMode::ASYNCHRON,
                { &aItem });
    }
    else if (sId == ".uno:DeleteAllAnnotationByAuthor")
    {
        const SfxStringItem aItem( SID_DELETEALLBYAUTHOR_POSTIT, sAuthor );
        pDispatcher->ExecuteList( SID_DELETEALLBYAUTHOR_POSTIT,
                SfxCallMode::ASYNCHRON, { &aItem });
    }
    else if (sId == ".uno:DeleteAllAnnotation")
        pDispatcher->Execute( SID_DELETEALL_POSTIT );
}

Color AnnotationManagerImpl::GetColor(sal_uInt16 aAuthorIndex)
{
    if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
    {
        static const Color aArrayNormal[] = {
            COL_AUTHOR1_NORMAL,     COL_AUTHOR2_NORMAL,     COL_AUTHOR3_NORMAL,
            COL_AUTHOR4_NORMAL,     COL_AUTHOR5_NORMAL,     COL_AUTHOR6_NORMAL,
            COL_AUTHOR7_NORMAL,     COL_AUTHOR8_NORMAL,     COL_AUTHOR9_NORMAL };

        return aArrayNormal[ aAuthorIndex % SAL_N_ELEMENTS( aArrayNormal ) ];
    }

    return COL_WHITE;
}

Color AnnotationManagerImpl::GetColorLight(sal_uInt16 aAuthorIndex)
{
    if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
    {
        static const Color aArrayLight[] = {
            COL_AUTHOR1_LIGHT,      COL_AUTHOR2_LIGHT,      COL_AUTHOR3_LIGHT,
            COL_AUTHOR4_LIGHT,      COL_AUTHOR5_LIGHT,      COL_AUTHOR6_LIGHT,
            COL_AUTHOR7_LIGHT,      COL_AUTHOR8_LIGHT,      COL_AUTHOR9_LIGHT };

        return aArrayLight[ aAuthorIndex % SAL_N_ELEMENTS( aArrayLight ) ];
    }

    return COL_WHITE;
}

Color AnnotationManagerImpl::GetColorDark(sal_uInt16 aAuthorIndex)
{
    if (!Application::GetSettings().GetStyleSettings().GetHighContrastMode())
    {
        static const Color aArrayAnkor[] = {
            COL_AUTHOR1_DARK,       COL_AUTHOR2_DARK,       COL_AUTHOR3_DARK,
            COL_AUTHOR4_DARK,       COL_AUTHOR5_DARK,       COL_AUTHOR6_DARK,
            COL_AUTHOR7_DARK,       COL_AUTHOR8_DARK,       COL_AUTHOR9_DARK };

        return aArrayAnkor[  aAuthorIndex % SAL_N_ELEMENTS( aArrayAnkor ) ];
    }

    return COL_WHITE;
}

SdPage* AnnotationManagerImpl::GetNextPage( SdPage const * pPage, bool bForward )
{
    if( pPage == nullptr )
    {
        if (bForward)
            return mpDoc->GetSdPage(0, PageKind::Standard ); // first page
        else
            return mpDoc->GetMasterSdPage( mpDoc->GetMasterSdPageCount(PageKind::Standard) - 1, PageKind::Standard ); // last page
    }

    sal_uInt16 nPageNum = (pPage->GetPageNum() - 1) >> 1;

    // first all non master pages
    if( !pPage->IsMasterPage() )
    {
        if( bForward )
        {
            if( nPageNum >= mpDoc->GetSdPageCount(PageKind::Standard)-1 )
            {
                // we reached end of draw pages, start with master pages (skip handout master for draw)
                return mpDoc->GetMasterSdPage( (mpDoc->GetDocumentType() == DocumentType::Impress) ? 0 : 1, PageKind::Standard );
            }
            nPageNum++;
        }
        else
        {
            if( nPageNum == 0 )
                return nullptr; // we are already on the first draw page, finished

            nPageNum--;
        }
        return mpDoc->GetSdPage(nPageNum, PageKind::Standard);
    }
    else
    {
        if( bForward )
        {
            if( nPageNum >= mpDoc->GetMasterSdPageCount(PageKind::Standard)-1 )
            {
                return nullptr;   // we reached the end, there is nothing more to see here
            }
            nPageNum++;
        }
        else
        {
            if( nPageNum == (mpDoc->GetDocumentType() == DocumentType::Impress ? 0 : 1) )
            {
                // we reached beginning of master pages, start with end if pages
                return mpDoc->GetSdPage( mpDoc->GetSdPageCount(PageKind::Standard)-1, PageKind::Standard );
            }

            nPageNum--;
        }
        return mpDoc->GetMasterSdPage(nPageNum,PageKind::Standard);
    }
}

SdPage* AnnotationManagerImpl::GetCurrentPage()
{
    if (mrBase.GetMainViewShell())
        return mrBase.GetMainViewShell()->getCurrentPage();
    return nullptr;
}

AnnotationManager::AnnotationManager( ViewShellBase& rViewShellBase )
: mxImpl( new AnnotationManagerImpl( rViewShellBase ) )
{
    mxImpl->init();
}

AnnotationManager::~AnnotationManager()
{
    mxImpl->dispose();
}

void AnnotationManager::ExecuteAnnotation(SfxRequest const & rRequest)
{
    mxImpl->ExecuteAnnotation( rRequest );
}

void AnnotationManager::GetAnnotationState(SfxItemSet& rItemSet)
{
    mxImpl->GetAnnotationState(rItemSet);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
