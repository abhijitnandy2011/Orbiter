
This is the basic testing skeleton. Its not yet fully functional.



----------------------------- Code Conventions--------------------------
1. We prefer starting function names with lower case as Orbiter functions use
upper case. Easier to distinguish.

2. We wil be adding the classes to their own namespace once its clear what they should be 
   - right now there arent enough classes to organize them into namespaces
   
3. Public member variables do not start with m_
4. namespace names are camel cased beginning with a capital letter
5. DirectX object accessor functions as can be found in D3D11Config reflect their function names
   as in their interface, exactly. 