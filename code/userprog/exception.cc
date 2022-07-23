// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
//#include "userkernel.h"


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
int count = 0;
void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val, status, fileID,numChar;
    
    switch (which) {

	case PageFaultException:
      {    
		  //hw3
       	count++;
		kernel->stats->numPageFaults = count;
        unsigned int vpn;
        int vadr = kernel->machine->ReadRegister(BadVAddrReg);
        vpn =  (unsigned) vadr / PageSize;
        char *buffer = new char[PageSize]; 
        
        kernel->swapFile->ReadAt(buffer, PageSize, kernel->currentThread->space->Swap[vpn]*PageSize);        

        int physicalPageNum = kernel->bitmap->FindAndSet(); 

        if (physicalPageNum != -1)
        {
            kernel->currentThread->space->setAttr(TRUE, physicalPageNum, vpn);   
			counter[physicalPageNum] = PagFrames;  
			PagFrames++;
            kernel->Phypage_ThreadMap.insert(pair<int, Thread*>(physicalPageNum, kernel->currentThread));
            bcopy (buffer, &kernel->machine->mainMemory[physicalPageNum *PageSize], PageSize);
        }
        else 
        {
			int LRU = 1e8;
			for(int i = 0; i < NumPhysPages; i++){
				if(counter[i] < LRU){
				LRU = counter[i];
				}
			}
            char *Buffer = new char[PageSize];

            bcopy (&kernel->machine->mainMemory[LRU *PageSize], Buffer, PageSize);
            kernel->bitmap->Clear(LRU);

            Thread *swapThread = kernel->Phypage_ThreadMap[LRU];
            
            int vpn_rep = swapThread->space->getVPN(LRU);
            if(vpn_rep!=-1)
            {
                int swapIndex = swapThread->space->Swap[vpn_rep];
                kernel->swapFile->WriteAt(Buffer, PageSize, swapIndex*PageSize);
                kernel->Phypage_ThreadMap.erase(LRU);
                
                int physicalPageNum = kernel->bitmap->FindAndSet(); 
                if (physicalPageNum != -1)
                {  
					kernel->currentThread->space->setAttr(TRUE, physicalPageNum, vpn);   
					counter[physicalPageNum] = PagFrames;  
					PagFrames++;         
                    kernel->Phypage_ThreadMap.insert(pair<int, Thread*>(physicalPageNum, kernel->currentThread));
                    bcopy (buffer, &kernel->machine->mainMemory[physicalPageNum *PageSize], PageSize);
                }
            }
            delete Buffer;
        }
    delete buffer;


		return;
    		
		ASSERTNOTREACHED();
        break;
	  }	  
	case SyscallException:
	    switch(type) {
		case SC_Halt:
		    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
   		    kernel->interrupt->Halt();
		    break;
		case SC_PrintInt:
			val=kernel->machine->ReadRegister(4);
			cout << "Print integer:" <<val << endl;
			return;
    		
		    ASSERTNOTREACHED();
            break;
			//return;
/*		case SC_Exec:
			DEBUG(dbgAddr, "Exec\n");
			val = kernel->machine->ReadRegister(4);
			kernel->StringCopy(tmpStr, retVal, 1024);
			cout << "Exec: " << val << endl;
			val = kernel->Exec(val);
			kernel->machine->WriteRegister(2, val);
			return;
*/		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
			val=kernel->machine->ReadRegister(4);
			cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			return;
    		
		    ASSERTNOTREACHED();
			
			break;
       

		case SC_Msg:
		{
			//DEBUG(dbgSys, "Message received.\n");
			val = kernel->machine->ReadRegister(4);
			{
				char *msg = &(kernel->machine->mainMemory[val]);
				cout << msg << endl;
			}
			kernel->interrupt->Halt();
			ASSERTNOTREACHED();
			break;
		}

		case SC_Create:
			val = kernel->machine->ReadRegister(4);
			{
				char *filename = &(kernel->machine->mainMemory[val]);
				status = kernel->fileSystem->Create(filename);	
				kernel->machine->WriteRegister(2, (int)status);
			}
			return;
			ASSERTNOTREACHED();
			break;
		//<TODO
		
		case SC_Open:
		val = kernel->machine->ReadRegister(4); 
		{
		
    		//cout << "val = " << val << endl;
    		char *filename = &(kernel->machine->mainMemory[val]);
    		//cout << "filename = " << filename << endl;
    		status = kernel->fileSystem->OpenAFile(filename); 
    		kernel->machine->WriteRegister(2, (int) status);
		}	
    	    return;	
    		ASSERTNOTREACHED();
            break;
		
		case SC_Read:
		 val = kernel->machine->ReadRegister(4);

        {
              char *buffer = &(kernel->machine->mainMemory[val]);
              numChar = kernel->machine->ReadRegister(5);
              status = kernel->fileSystem->ReadFile(buffer,numChar);
              kernel->machine->WriteRegister(2, (int) status);          
        }

                    return;

                    ASSERTNOTREACHED();

                    break;

		case SC_Write:
	     val = kernel->machine->ReadRegister(4);

                    {
                        char *buffer = &(kernel->machine->mainMemory[val]);
                        numChar = kernel->machine->ReadRegister(5);
                        status =kernel->fileSystem->WriteFile1(buffer, numChar);
                        kernel->machine->WriteRegister(2, (int) status);
                    }


                    return;

                    ASSERTNOTREACHED();
	
		           break;
		case SC_Close:
        val = kernel->machine->ReadRegister(4);
	    
          {  
			 status = kernel->fileSystem->CloseFile();
             kernel->machine->WriteRegister(2, (int) status);
			// cout<<"hey!"<<endl;
		  }	
           
    		return;
    		
		ASSERTNOTREACHED();
        break;

        
		//TODO>
		default:
		    cerr << "Unexpected system call " << type << "\n";
 		    break;
		
	    }
	    break;
	default:
	    cerr << "Unexpected user mode exception" << which << "\n";
	    break;
    }
    ASSERTNOTREACHED();
}
