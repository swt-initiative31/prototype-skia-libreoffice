package start;

import com.sun.star.awt.XToolkit;
import com.sun.star.lang.XMultiComponentFactory;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XComponentContext;

public class MinVclStart {

	public static void main(String[] args) {

		try {
			XComponentContext xContext = MinVclBootstrap.bootstrap();
			if (xContext != null)
				System.out.println("Connected to a running office ...");
			XMultiComponentFactory xMCF = xContext.getServiceManager();

			Object oToolkit = xMCF.createInstanceWithContext("com.sun.star.awt.Toolkit", xContext);
			XToolkit xToolkit = UnoRuntime.queryInterface(XToolkit.class, oToolkit);
		
		
			long timeMillis = System.currentTimeMillis();
			
			while(System.currentTimeMillis() - timeMillis < 100000) {
				
				System.out.println("waiting...");
				
				Thread.sleep(1000);
				
			}
			
		
		}catch(Exception e) {
			e.printStackTrace(System.err);
		}
			
			
	}

}
