
/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 *  The Contents of this file are made available subject to the terms of
 *  the BSD license.
 *
 *  Copyright 2000, 2010 Oracle and/or its affiliates.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Sun Microsystems, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************/

package main;

import com.sun.star.awt.ActionEvent;
import com.sun.star.awt.AdjustmentEvent;
import com.sun.star.awt.AdjustmentType;
import com.sun.star.awt.FocusChangeReason;
import com.sun.star.awt.FocusEvent;
import com.sun.star.awt.InvalidateStyle;
import com.sun.star.awt.ItemEvent;
import com.sun.star.awt.KeyEvent;
import com.sun.star.awt.MouseEvent;
import com.sun.star.awt.Rectangle;
import com.sun.star.awt.SpinEvent;
import com.sun.star.awt.TextEvent;
import com.sun.star.awt.XActionListener;
import com.sun.star.awt.XAdjustmentListener;
import com.sun.star.awt.XButton;
import com.sun.star.awt.XCheckBox;
import com.sun.star.awt.XComboBox;
import com.sun.star.awt.XControl;
import com.sun.star.awt.XControlContainer;
import com.sun.star.awt.XControlModel;
import com.sun.star.awt.XDialog;
import com.sun.star.awt.XFixedText;
import com.sun.star.awt.XFocusListener;
import com.sun.star.awt.XItemEventBroadcaster;
import com.sun.star.awt.XItemListener;
import com.sun.star.awt.XKeyListener;
import com.sun.star.awt.XListBox;
import com.sun.star.awt.XMouseListener;
import com.sun.star.awt.XPointer;
import com.sun.star.awt.XRegion;
import com.sun.star.awt.XReschedule;
import com.sun.star.awt.XScrollBar;
import com.sun.star.awt.XSpinField;
import com.sun.star.awt.XSpinListener;
import com.sun.star.awt.XTextComponent;
import com.sun.star.awt.XTextListener;
import com.sun.star.awt.XToolkit;
import com.sun.star.awt.XTopWindow;
import com.sun.star.awt.XView;
import com.sun.star.awt.XWindow;
import com.sun.star.awt.XWindowPeer;
import com.sun.star.beans.XMultiPropertySet;
import com.sun.star.beans.XPropertySet;
import com.sun.star.container.XIndexContainer;
import com.sun.star.container.XNameAccess;
import com.sun.star.container.XNameContainer;
import com.sun.star.frame.XDesktop;
import com.sun.star.frame.XFrame;
import com.sun.star.frame.XModel;
import com.sun.star.lang.EventObject;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiComponentFactory;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.lang.XSingleServiceFactory;
import com.sun.star.ucb.XFileIdentifierConverter;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XComponentContext;
import com.sun.star.util.Date;
import com.sun.star.util.Time;
import com.sun.star.util.XNumberFormats;
import com.sun.star.util.XNumberFormatsSupplier;
import com.sun.star.util.thePathSettings;

import start.MinVclBootstrap;

// Suggestion: hand the position and width over as parameters

public class TextInMainControl {

	public static <T> T qi(Class<T> aType, Object o)
	{
		return UnoRuntime.queryInterface(aType, o);
	}

	public static void main(String args[]) {
		TextInMainControl oUnoDialogSample = null;

		try {
			XComponentContext xContext = MinVclBootstrap.bootstrap();
			if (xContext != null)
				System.out.println("Connected to a running office ...");
			XMultiComponentFactory xMCF = xContext.getServiceManager();

			Object oToolkit = xMCF.createInstanceWithContext("com.sun.star.awt.Toolkit", xContext);
			XToolkit xToolkit = qi(XToolkit.class, oToolkit);

			Object oDesktop = xMCF.createInstanceWithContext("com.sun.star.frame.Desktop", xContext);
			XDesktop xDesktop = qi(XDesktop.class, oDesktop);

			XWindow w = xDesktop.getCurrentFrame().getContainerWindow();
			
			
			XWindowPeer xWindowPeer = UnoRuntime.queryInterface(XWindowPeer.class, w);
			
			
			int nColor0 = (0 << 16) | (255 << 8) | 0;
			xWindowPeer.setBackground(nColor0);


			XControlContainer c = UnoRuntime.queryInterface(XControlContainer.class, w);

			Rectangle wa = xToolkit.getWorkArea();
			System.out.println("Workarea: " + wa.Height + "  " + wa.Width);

			XTopWindow xTopWindow = UnoRuntime.queryInterface(XTopWindow.class, w);

			XControlContainer xControlContainer = UnoRuntime.queryInterface(XControlContainer.class, w);

			if (xControlContainer != null) {

				xControlContainer.getControls();
			} else
				System.out.println("No control container...");

			// Erzeugung des FixedText Models
			Object fixedTextModel = xMCF.createInstanceWithContext("com.sun.star.awt.UnoControlFixedTextModel",
					xContext);
			XPropertySet propFixedText = UnoRuntime.queryInterface(XPropertySet.class, fixedTextModel);

			// Setze die Eigenschaften des Texts
			// Das scheint nicht richtig zu funktionieren, wir müssen später die position nochmal setzen.
			propFixedText.setPropertyValue("PositionX", 500);
			propFixedText.setPropertyValue("PositionY", 500);
			propFixedText.setPropertyValue("Width", 800);
			propFixedText.setPropertyValue("Height", 400);
			propFixedText.setPropertyValue("Name", "myText");
			propFixedText.setPropertyValue("Label", "Dies ist ein Text");
			
//			propFixedText.getPropertySetInfo();

			// Erzeugung der UnoControlFixedText-Instanz und Einsetzen des FixedText-Models
			Object oControl = xMCF.createInstanceWithContext("com.sun.star.awt.UnoControlFixedText", xContext);
			XControl xControl = UnoRuntime.queryInterface(XControl.class, oControl);
			// das Modell setzen
			xControl.setModel(UnoRuntime.queryInterface(XControlModel.class, fixedTextModel));
			
			XWindowPeer peer2 = xControl.getPeer();
			
			// test for setting colors: blue
			int color4 = (0 << 16) | (0 << 8) | 255;
			xWindowPeer.setBackground(color4);
			
			
			xControl.createPeer(xToolkit, xWindowPeer);
			
			
			
	//		-------------------------------------- just some testing, not important...
			XView view = xControl.getView();
			if(view != null )
				System.out.println("view: " + view.getSize().Height + "  " + view.getSize().Width );
			else
				System.out.println("view is null");
			
			
			Object context = xControl.getContext();
			System.out.println("Context: " + context);
			
			XControlModel model = xControl.getModel();
			System.out.println("XControlModel: " + model);
			
			XWindowPeer peer = xControl.getPeer();
			
			XWindow peerWin = UnoRuntime.queryInterface(XWindow.class, peer  );
			
			// There seems to be a bug, we have to reset the position...
			peerWin.setPosSize(100, 100, 500, 500, com.sun.star.awt.PosSize.POSSIZE);
			
			
			System.out.println("Peer: " + peer);
			int nColor1 = (0 << 16) | (255 << 8) | 0;
			peer.setBackground(nColor1);
			
			XRegion region = peer.getToolkit().createRegion();
			
			
			int nColor2 = (255 << 16) | (0 << 8) | 0; 
			xWindowPeer.setBackground(nColor2);
			
			// xWindowPeer ist eine Instanz von XWindowPeer
			// Stellen Sie sicher, dass das Fenster neu gezeichnet wird
			xWindowPeer.invalidateRect( new Rectangle(50, 50, 100, 100) , InvalidateStyle.TRANSPARENT  );
			xWindowPeer.invalidateRect( new Rectangle(50, 50, 1000, 1000) , InvalidateStyle.UPDATE  );
			
			
			//--------------------------------------------
			
		

//			propFixedText.setPropertyValue("Width", 800);
//			propFixedText.setPropertyValue("Height", 400);
//			
			xWindowPeer.invalidateRect( new Rectangle(0, 0, 1000, 1000) , InvalidateStyle.UPDATE  );
			
			
			
			
			

		} catch (Exception e) {
			System.err.println(e + e.getMessage());
			e.printStackTrace();
		}  

		System.exit(0);
	}


}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
