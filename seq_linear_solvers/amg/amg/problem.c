/*BHEADER**********************************************************************
 * (c) 1996   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/

/******************************************************************************
 *
 * Constructors and destructors for problem structure.
 *
 *****************************************************************************/

#include "amg.h"


/*--------------------------------------------------------------------------
 * NewProblem
 *--------------------------------------------------------------------------*/

Problem  *NewProblem(file_name)
char     *file_name;
{
   Problem  *problem;

   FILE     *fp;

   int      num_variables;

   Matrix  *A;
   Vector  *f;
   Vector  *u;

   int      num_unknowns;
   int      num_points;

   int     *iu;
   int     *ip;
   int     *iv;

   double  *x;
   double  *y;
   double  *z;

   char     temp_file_name[256];
   FILE    *temp_fp;
   int      flag;
   double  *data;

   int      i, j, k;


   /*----------------------------------------------------------
    * Open the problem file
    *----------------------------------------------------------*/

   fp = fopen(file_name, "r");

   /*----------------------------------------------------------
    * num_variables
    *----------------------------------------------------------*/

   fscanf(fp, "%d", &num_variables);

   /*----------------------------------------------------------
    * A
    *----------------------------------------------------------*/

   fscanf(fp, "%s", temp_file_name);
   A = ReadYSMP(temp_file_name);

   if (MatrixSize(A) != num_variables)
   {
      printf("Matrix is of incompatible size\n");
      exit(1);
   }

   /*----------------------------------------------------------
    * f
    *----------------------------------------------------------*/

   fscanf(fp, "%d", &flag);

   if (flag == 0)
   {
      fscanf(fp, "%s", temp_file_name);
      f = ReadVec(temp_file_name);

      if (VectorSize(f) != num_variables)
      {
	 printf("Right hand side is of incompatible\n");
	 exit(1);
      }
   }
   else
   {
      data = talloc(double, NDIMU(num_variables));
      for (i = 0; i < num_variables; i++)
	 data[i] = 0.0;
      f = NewVector(data, num_variables);
   }

   /*----------------------------------------------------------
    * u
    *----------------------------------------------------------*/

   fscanf(fp, "%d", &flag);

   if (flag == 0)
   {
      fscanf(fp, "%s", temp_file_name);
      u = ReadVec(temp_file_name);

      if (VectorSize(u) != num_variables)
      {
	 printf("Initial guess is of incompatible size\n");
	 exit(1);
      }
   }
   else
   {
      SeedRand(1);
      data = talloc(double, NDIMU(num_variables));
      for (i = 0; i < num_variables; i++)
	 data[i] = Rand();
      u = NewVector(data, num_variables);
   }

   /*----------------------------------------------------------
    * num_unknowns, num_points
    *----------------------------------------------------------*/

   fscanf(fp, "%d%d", &num_unknowns, &num_points);

   /*----------------------------------------------------------
    * iu, ip, iv
    *----------------------------------------------------------*/

   iu = talloc(int, NDIMU(num_variables));
   ip = talloc(int, NDIMU(num_variables));
   iv = talloc(int, NDIMP(num_points+1));

   fscanf(fp, "%d", &flag);

   if (flag == 0)
   {
      for (j = 0; j < num_variables; j++)
	 fscanf(fp, "%d", &iu[j]);
      for (j = 0; j < num_variables; j++)
	 fscanf(fp, "%d", &ip[j]);
      for (j = 0; j < num_points+1; j++)
	 fscanf(fp, "%d", &iv[j]);
   }
   else
   {
      if ((num_points*num_unknowns) != num_variables)
      {
	 printf("Incompatible number of points, unknowns, and variables\n");
	 exit(1);
      }

      k = 0;
      for (i = 1; i <= num_points; i++)
	 for (j = 1; j <= num_unknowns; j++)
	 {
	    iu[k] = j;
	    ip[k] = i;
	    k++;
	 }
      i = 1;
      for (k = 0; k <= num_points; k++)
      {
	 iv[k] = i;
	 i += num_unknowns;
      }
   }

   /*----------------------------------------------------------
    * x, y, z
    *----------------------------------------------------------*/

   x = talloc(double, NDIMP(num_points));
   y = talloc(double, NDIMP(num_points));
   z = talloc(double, NDIMP(num_points));

   fscanf(fp, "%d", &flag);

   if (flag == 0)
   {
      fscanf(fp, "%s", temp_file_name);
      temp_fp = fopen(temp_file_name, "r");

      for (j = 0; j < num_points; j++)
	 fscanf(temp_fp, "%le", &x[j]);
      for (j = 0; j < num_points; j++)
	 fscanf(temp_fp, "%le", &y[j]);
      for (j = 0; j < num_points; j++)
	 fscanf(temp_fp, "%le", &z[j]);

      fclose(temp_fp);
   }

   /*----------------------------------------------------------
    * Close the problem file
    *----------------------------------------------------------*/

   fclose(fp);

   /*----------------------------------------------------------
    * Set the problem structure
    *----------------------------------------------------------*/

   problem = talloc(Problem, 1);

   ProblemNumVariables(problem) = num_variables;

   ProblemA(problem)   		= A;
   ProblemF(problem)   		= f;
   ProblemU(problem)   		= u;

   ProblemNumUnknowns(problem)  = num_unknowns;
   ProblemNumPoints(problem)    = num_points;

   ProblemIU(problem)           = iu;
   ProblemIP(problem)           = ip;
   ProblemIV(problem)           = iv;

   ProblemX(problem)            = x;
   ProblemY(problem)            = y;
   ProblemZ(problem)            = z;

   return problem;
}

/*--------------------------------------------------------------------------
 * FreeProblem
 *--------------------------------------------------------------------------*/

void     FreeProblem(problem)
Problem  *problem;
{
   if (problem)
   {
      tfree(ProblemIU(problem));
      tfree(ProblemIP(problem));
      tfree(ProblemIV(problem));
      tfree(ProblemX(problem));
      tfree(ProblemY(problem));
      tfree(ProblemZ(problem));
      tfree(problem);
   }
}

