#include <inc/lib.h>

//Functions Declarations
void InitializeAscending(int **Elements, int NumOfElements);
void InitializeIdentical(int **Elements, int NumOfElements, int value);
void InitializeSemiRandom(int **Elements, int NumOfElements);
void PrintElements(int **Elements, int NumOfElements);
void PrintElements64(int64 **Elements, int NumOfElements);

int64** MatrixMultiply(int **M1, int **M2, int NumOfElements);
int64** MatrixAddition(int **M1, int **M2, int NumOfElements);
int64** MatrixSubtraction(int **M1, int **M2, int NumOfElements);

void _main(void)
{
	char Line[255] ;
	char Chose ;
	int val =0 ;
	int NumOfElements = 3;
	do
	{
		val = 0;
		NumOfElements = 3;
		//2012: lock the interrupt
		sys_lock_cons();
		cprintf("\n");
		cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		cprintf("!!!   MATRIX MULTIPLICATION    !!!\n");
		cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		cprintf("\n");

		readline("Enter the number of elements: ", Line);
		NumOfElements = strtol(Line, NULL, 10) ;

		cprintf("Chose the initialization method:\n") ;
		cprintf("a) Ascending\n") ;
		cprintf("b) Identical\n") ;
		cprintf("c) Semi random\n");
		do
		{
			cprintf("Select: ") ;
			Chose = getchar() ;
			cputchar(Chose);
			cputchar('\n');
		} while (Chose != 'a' && Chose != 'b' && Chose != 'c');

		if (Chose == 'b')
		{
			readline("Enter the value to be initialized: ", Line);
			val = strtol(Line, NULL, 10) ;
		}
		//2012: lock the interrupt
		sys_unlock_cons();

		int **M1 = malloc(sizeof(int) * NumOfElements) ;
		int **M2 = malloc(sizeof(int) * NumOfElements) ;

		for (int i = 0; i < NumOfElements; ++i)
		{
			M1[i] = malloc(sizeof(int) * NumOfElements) ;
			M2[i] = malloc(sizeof(int) * NumOfElements) ;
		}

		int  i ;
		switch (Chose)
		{
		case 'a':
			InitializeAscending(M1, NumOfElements);
			InitializeAscending(M2, NumOfElements);
			break ;
		case 'b':
			InitializeIdentical(M1, NumOfElements, val);
			InitializeIdentical(M2, NumOfElements, val);
			break ;
		case 'c':
			InitializeSemiRandom(M1, NumOfElements);
			InitializeSemiRandom(M2, NumOfElements);
			//PrintElements(M1, NumOfElements);
			break ;
		default:
			InitializeSemiRandom(M1, NumOfElements);
			InitializeSemiRandom(M2, NumOfElements);
		}

		sys_lock_cons();
		cprintf("Chose the desired operation:\n") ;
		cprintf("a) Addition       (+)\n") ;
		cprintf("b) Subtraction    (-)\n") ;
		cprintf("c) Multiplication (x)\n");
		do
		{
			cprintf("Select: ") ;
			Chose = getchar() ;
			cputchar(Chose);
			cputchar('\n');
		} while (Chose != 'a' && Chose != 'b' && Chose != 'c');
		sys_unlock_cons();


		int64** Res = NULL ;
		switch (Chose)
		{
		case 'a':
			Res = MatrixAddition(M1, M2, NumOfElements);
			//PrintElements64(Res, NumOfElements);
			break ;
		case 'b':
			Res = MatrixSubtraction(M1, M2, NumOfElements);
			//PrintElements64(Res, NumOfElements);
			break ;
		case 'c':
			Res = MatrixMultiply(M1, M2, NumOfElements);
			//PrintElements64(Res, NumOfElements);
			break ;
		default:
			Res = MatrixAddition(M1, M2, NumOfElements);
			//PrintElements64(Res, NumOfElements);
		}


		sys_lock_cons();
		cprintf("Operation is COMPLETED.\n");
		sys_unlock_cons();

		for (int i = 0; i < NumOfElements; ++i)
		{
			free(M1[i]);
			free(M2[i]);
			free(Res[i]);
		}
		free(M1) ;
		free(M2) ;
		free(Res) ;


		sys_lock_cons();
		cprintf("Do you want to repeat (y/n): ") ;
		Chose = getchar() ;
		cputchar(Chose);
		cputchar('\n');
		sys_unlock_cons();

	} while (Chose == 'y');

}

///MATRIX MULTIPLICATION
int64** MatrixMultiply(int **M1, int **M2, int NumOfElements)
{
	int64 **Res = malloc(sizeof(int64) * NumOfElements) ;
	for (int i = 0; i < NumOfElements; ++i)
	{
		Res[i] = malloc(sizeof(int64) * NumOfElements) ;
	}

	for (int i = 0; i < NumOfElements; ++i)
	{
		for (int j = 0; j < NumOfElements; ++j)
		{
			Res[i][j] = 0 ;
			for (int k = 0; k < NumOfElements; ++k)
			{
				Res[i][j] += M1[i][k] * M2[k][j] ;
			}
		}
	}
	return Res;
}

///MATRIX ADDITION
int64** MatrixAddition(int **M1, int **M2, int NumOfElements)
{
	int64 **Res = malloc(sizeof(int64) * NumOfElements) ;
	for (int i = 0; i < NumOfElements; ++i)
	{
		Res[i] = malloc(sizeof(int64) * NumOfElements) ;
	}

	for (int i = 0; i < NumOfElements; ++i)
	{
		for (int j = 0; j < NumOfElements; ++j)
		{
			Res[i][j] = M1[i][j] + M2[i][j] ;
		}
	}
	return Res;
}

///MATRIX SUBTRACTION
int64** MatrixSubtraction(int **M1, int **M2, int NumOfElements)
{
	int64 **Res = malloc(sizeof(int64) * NumOfElements) ;
	for (int i = 0; i < NumOfElements; ++i)
	{
		Res[i] = malloc(sizeof(int64) * NumOfElements) ;
	}

	for (int i = 0; i < NumOfElements; ++i)
	{
		for (int j = 0; j < NumOfElements; ++j)
		{
			Res[i][j] = M1[i][j] - M2[i][j] ;
		}
	}
	return Res;
}

///Private Functions

void InitializeAscending(int **Elements, int NumOfElements)
{
	int i, j ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		for (j = 0 ; j < NumOfElements ; j++)
		{
			(Elements)[i][j] = j ;
		}
	}
}

void InitializeIdentical(int **Elements, int NumOfElements, int value)
{
	int i, j ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		for (j = 0 ; j < NumOfElements ; j++)
		{
			(Elements)[i][j] = value ;
		}
	}
}

void InitializeSemiRandom(int **Elements, int NumOfElements)
{
	int i, j ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		for (j = 0 ; j < NumOfElements ; j++)
		{
			(Elements)[i][j] =  RAND(0, NumOfElements) ;
			//	cprintf("i=%d\n",i);
		}
	}
}

void PrintElements(int **Elements, int NumOfElements)
{
	int i, j ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		for (j = 0 ; j < NumOfElements ; j++)
		{
			cprintf("%~%d, ",Elements[i][j]);
		}
		cprintf("%~\n");
	}
}

void PrintElements64(int64 **Elements, int NumOfElements)
{
	int i, j ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		for (j = 0 ; j < NumOfElements ; j++)
		{
			cprintf("%~%lld, ",Elements[i][j]);
		}
		cprintf("%~\n");
	}
}
