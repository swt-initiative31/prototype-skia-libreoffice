package main;

import com.sun.star.beans.PropertyValue;
import com.sun.star.beans.XPropertySet;
import com.sun.star.uno.XComponentContext;
import com.sun.star.comp.helper.Bootstrap;
import com.sun.star.container.XEnumeration;
import com.sun.star.container.XEnumerationAccess;
import com.sun.star.frame.XComponentLoader;
import com.sun.star.frame.XController;
import com.sun.star.frame.XModel;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiComponentFactory;
import com.sun.star.sheet.XCellAddressable;
import com.sun.star.sheet.XCellRangesQuery;
import com.sun.star.sheet.XSheetCellRanges;
import com.sun.star.sheet.XSpreadsheet;
import com.sun.star.sheet.XSpreadsheetDocument;
import com.sun.star.sheet.XSpreadsheetView;
import com.sun.star.sheet.XSpreadsheets;
import com.sun.star.table.XCell;
import com.sun.star.uno.UnoRuntime;

public class FirstLoadComponent {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        try {
            // get the remote office component context
            XComponentContext xRemoteContext = Bootstrap.bootstrap();
            if (xRemoteContext == null) {
                System.err.println("ERROR: Could not bootstrap default Office.");
            }

            XMultiComponentFactory xRemoteServiceManager = xRemoteContext.getServiceManager();

            Object desktop = xRemoteServiceManager.createInstanceWithContext(
                "com.sun.star.frame.Desktop", xRemoteContext);
            XComponentLoader xComponentLoader = UnoRuntime.queryInterface(XComponentLoader.class, desktop);

            PropertyValue[] loadProps = new PropertyValue[0];
            XComponent xSpreadsheetComponent = xComponentLoader.loadComponentFromURL("private:factory/scalc", "_blank", 0, loadProps);

            XSpreadsheetDocument xSpreadsheetDocument = UnoRuntime.queryInterface(XSpreadsheetDocument.class,
                                      xSpreadsheetComponent);

            XSpreadsheets xSpreadsheets = xSpreadsheetDocument.getSheets();
            xSpreadsheets.insertNewByName("MySheet", (short)0);
            com.sun.star.uno.Type elemType = xSpreadsheets.getElementType();

            System.out.println(elemType.getTypeName());
            Object sheet = xSpreadsheets.getByName("MySheet");
            XSpreadsheet xSpreadsheet = UnoRuntime.queryInterface(
                XSpreadsheet.class, sheet);

            XCell xCell = xSpreadsheet.getCellByPosition(0, 0);
            xCell.setValue(21);
            xCell = xSpreadsheet.getCellByPosition(0, 1);
            xCell.setValue(21);
            xCell = xSpreadsheet.getCellByPosition(0, 2);
            xCell.setFormula("=sum(A1:A2)");

            XPropertySet xCellProps = UnoRuntime.queryInterface(
                XPropertySet.class, xCell);
            xCellProps.setPropertyValue("CellStyle", "Result");

            XModel xSpreadsheetModel = UnoRuntime.queryInterface(
                XModel.class, xSpreadsheetComponent);
            XController xSpreadsheetController = xSpreadsheetModel.getCurrentController();
            XSpreadsheetView xSpreadsheetView = UnoRuntime.queryInterface(XSpreadsheetView.class,
                                      xSpreadsheetController);
            xSpreadsheetView.setActiveSheet(xSpreadsheet);

            // *********************************************************
            // example for use of enum types
            xCellProps.setPropertyValue("VertJustify",
                                        com.sun.star.table.CellVertJustify.TOP);


            // *********************************************************
            // example for a sequence of PropertyValue structs
            // create an array with one PropertyValue struct, it contains
            // references only
            loadProps = new PropertyValue[1];

            // instantiate PropertyValue struct and set its member fields
            PropertyValue asTemplate = new PropertyValue();
            asTemplate.Name = "AsTemplate";
            asTemplate.Value = Boolean.TRUE;

            // assign PropertyValue struct to array of references for PropertyValue
            // structs
            loadProps[0] = asTemplate;

            // load calc file as template
            //xSpreadsheetComponent = xComponentLoader.loadComponentFromURL(
            //    "file:///c:/temp/DataAnalysys.ods", "_blank", 0, loadProps);

            // *********************************************************
            // example for use of XEnumerationAccess
            XCellRangesQuery xCellQuery = UnoRuntime.queryInterface(XCellRangesQuery.class, sheet);
            XSheetCellRanges xFormulaCells = xCellQuery.queryContentCells(
                (short)com.sun.star.sheet.CellFlags.FORMULA);
            XEnumerationAccess xFormulas = xFormulaCells.getCells();
            XEnumeration xFormulaEnum = xFormulas.createEnumeration();

            while (xFormulaEnum.hasMoreElements()) {
                Object formulaCell = xFormulaEnum.nextElement();
                xCell = UnoRuntime.queryInterface(XCell.class, formulaCell);
                XCellAddressable xCellAddress = UnoRuntime.queryInterface(XCellAddressable.class, xCell);
                System.out.println("Formula cell in column " +
                                   xCellAddress.getCellAddress().Column
                                   + ", row " + xCellAddress.getCellAddress().Row
                                   + " contains " + xCell.getFormula());
            }

        }
        catch (java.lang.Exception e){
            e.printStackTrace();
        }
        finally {
            System.exit( 0 );
        }
    }

}