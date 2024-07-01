package pipetest;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class ExecuteOfficePipe {

	 public static final String[] getDefaultOptions()
	 {
	     return new String[]
	     {
	         "--nologo",
	         "--nodefault",
	         "--norestore",
	         "--nolockcheck"
	     };
	 }
	
	private static final Random randomPipeName = new Random();

	public static void main(String[] args) throws Exception {
		
		String[] defaultArgArray = getDefaultOptions();
		

// 		String exec = "/home/linuxsub/git/prototype-skia-libreoffice/instdir/program/oosplash";
		String exec = "/home/linuxsub/git/prototype-skia-libreoffice/workdir/LinkTarget/Executable/minvcl";
		String sPipeName = "uno" + Long.toString(randomPipeName.nextLong() & 0x7fffffffffffffffL);

 		String pipeArg = ("--accept=pipe,name=" + sPipeName + ";urp;");


		List<String> a = new ArrayList<String>();
		
		a.add(exec);
//		a.addAll(Arrays.asList(defaultArgArray));
		
 		a.add(pipeArg);

		System.out.println(Arrays.asList(a));

		ProcessBuilder b = new ProcessBuilder();

		// this doesn't work, don't know why...
//		b.environment().put("URE_BOOTSTRAP",
//				"/home/linuxsub/git/prototype-skia-libreoffice/instdir/program/fundamentalrc");
//		b.environment().put("LD_LIBRARY_PATH", "LD_LIBRARY_PATH:/home/linuxsub/git/prototype-skia-libreoffice/instdir/program");
		b.directory(new File("/home/linuxsub/git/prototype-skia-libreoffice/"));
		
		
		
		System.out.println(b.environment());
		
//
//		b.command(a);
//
//		Process p = b.start();
//		
	
		
		

		// start office process
        Process p = Runtime.getRuntime().exec( a.toArray(new String[0]) );
		pipe(p.getInputStream(), System.out, "CO> ");
		pipe(p.getErrorStream(), System.err, "CE> ");
		// start office process

		long timeMillis = System.currentTimeMillis();

		while (p.isAlive()) {

			System.out.println(p.info());
			System.out.println("waiting...");

			Thread.sleep(1000);

		}

	}

	private static void pipe(final InputStream in, final PrintStream out, final String prefix) {

		new Thread("Pipe: " + prefix) {
			@Override
			public void run() {
				try {
					BufferedReader r = new BufferedReader(new InputStreamReader(in, "UTF-8"));

					for (;;) {
						String s = r.readLine();
						if (s == null) {
							break;
						}
						out.println(prefix + s);
					}
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace(System.err);
				} catch (java.io.IOException e) {
					e.printStackTrace(System.err);
				}
			}
		}.start();
	}

}
