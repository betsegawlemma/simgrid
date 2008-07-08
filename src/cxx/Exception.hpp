/*
 * Exception.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_EXCEPTION_HPP
#define MSG_EXCEPTION_HPP

#ifndef __cplusplus
	#error Exception.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <Config.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		class SIMGRIDX_EXPORT Exception
		{
			public:
			
			// Default constructor.
				Exception();
			
			// Copy constructor.
				Exception(const Exception& rException);
			
			// This constructor takes the reason of the exception.
				Exception(const char* reason);
			
			// Destructor.
				virtual ~Exception();
				
			// Operations.
					
					// Returns the reason of the exception.
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const Exception& operator = (const Exception& rException);
				
			private :
			
			// Attributes.
				
				// The reason of the exceptions.
				const char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_EXCEPTION_HPP

