package start;

import com.sun.star.lang.XComponent;
import com.sun.star.uno.XComponentContext;

public class ComponentContextEntry  
{
    /** if late init of service instance, set service name (String) or
        component factory (XSingleComponentFactory), null otherwise
    */
    public Object m_lateInit;
    /** set entry value
    */
    public Object m_value;

    /** Creating a late-init singleton entry component context entry.
        The second parameter will be ignored and overwritten during
        instantiation of the singleton instance.

        @param lateInit
               object factory or service string
        @param value
               pass null (dummy separating from second constructor signature)
    */
    public ComponentContextEntry( Object lateInit, Object value )
    {
        this.m_lateInit = lateInit;
        this.m_value = value;
    }
    /** Creating a direct value component context entry.

        @param value
               pass null
    */
    public ComponentContextEntry( Object value )
    {
        this.m_lateInit = null;
        this.m_value = value;
    }
}