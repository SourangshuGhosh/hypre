/*BHEADER**********************************************************************
 * (c) 1997   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 * 
 *****************************************************************************/

#include "headers.h"

/*--------------------------------------------------------------------------
 * hypre_GetComputeInfo:
 *--------------------------------------------------------------------------*/

int
hypre_GetComputeInfo( hypre_BoxArrayArray  **send_boxes_ptr,
                      hypre_BoxArrayArray  **recv_boxes_ptr,
                      int                 ***send_processes_ptr,
                      int                 ***recv_processes_ptr,
                      hypre_BoxArrayArray  **indt_boxes_ptr,
                      hypre_BoxArrayArray  **dept_boxes_ptr,
                      hypre_StructGrid      *grid,
                      hypre_StructStencil   *stencil            )
{
   int                      ierr = 0;

   /* output variables */
   hypre_BoxArrayArray     *send_boxes;
   hypre_BoxArrayArray     *recv_boxes;
   int                    **send_processes;
   int                    **recv_processes;
   hypre_BoxArrayArray     *indt_boxes;
   hypre_BoxArrayArray     *dept_boxes;

   /* internal variables */
   hypre_BoxArray          *boxes;

   hypre_Box               *box0;

   int                      i;

#ifdef OVERLAP_COMM_COMP
   hypre_BoxArray          *send_box_a;
   hypre_BoxArray          *recv_box_a;
   hypre_BoxArray          *indt_box_a;
   hypre_BoxArray          *dept_box_a;

   hypre_BoxArrayArray     *box_aa0;
   hypre_BoxArray          *box_a0;
   hypre_BoxArray          *box_a1;
                         
   int                      j, k;
#endif

   /*------------------------------------------------------
    * Extract needed grid info
    *------------------------------------------------------*/

   boxes = hypre_StructGridBoxes(grid);

   /*------------------------------------------------------
    * Get communication info
    *------------------------------------------------------*/

   hypre_NewCommInfoFromStencil(&send_boxes, &recv_boxes,
                                &send_processes, &recv_processes,
                                grid, stencil );

   /*------------------------------------------------------
    * Set up the dependent boxes
    *------------------------------------------------------*/

   dept_boxes = hypre_NewBoxArrayArray(hypre_BoxArraySize(boxes));

#ifdef OVERLAP_COMM_COMP

   hypre_ForBoxI(i, boxes)
      {
         /* grow `recv_boxes' by stencil transpose to get `box_aa0' */
         recv_box_a = hypre_BoxArrayArrayBoxArray(recv_boxes, i);
         box_aa0 = hypre_GrowBoxArrayByStencil(recv_box_a, stencil, 1);

         /* intersect `box_aa0' with `boxes' to create `dept_box_a' */
         dept_box_a = hypre_NewBoxArray(0);
         box0 = hypre_NewBox();
         hypre_ForBoxArrayI(j, box_aa0)
            {
               box_a0 = hypre_BoxArrayArrayBoxArray(box_aa0, i);

               hypre_ForBoxI(k, box_a0)
                  {
                     hypre_IntersectBoxes(hypre_BoxArrayBox(box_a0, k),
                                          hypre_BoxArrayBox(boxes, i), box0);
                     if (hypre_BoxVolume(box0))
                     {
                        hypre_AppendBox(box0, dept_box_a);
                     }
                  }
            }
         hypre_FreeBox(box0);
         hypre_FreeBoxArrayArray(box_aa0);

         /* append `send_boxes' to `dept_box_a' */
         send_box_a = hypre_BoxArrayArrayBoxArray(send_boxes, i);
         hypre_ForBoxI(j, send_box_a)
            {
               box0 = hypre_BoxArrayBox(send_box_a, j);
               hypre_AppendBox(box0, dept_box_a);
            }

         /* union `dept_box_a' to minimize size of `dept_boxes' */
         hypre_BoxArrayArrayBoxArray(dept_boxes, i) =
            hypre_UnionBoxArray(dept_box_a);

         hypre_FreeBoxArray(dept_box_a);
         hypre_FreeBoxArrayArray(box_aa0);
      }

#else

   hypre_ForBoxI(i, boxes)
      {
         box0 = hypre_BoxArrayBox(boxes, i);
         hypre_AppendBox(box0, hypre_BoxArrayArrayBoxArray(dept_boxes, i));
      }

#endif

   /*------------------------------------------------------
    * Set up the independent boxes
    *------------------------------------------------------*/

   indt_boxes = hypre_NewBoxArrayArray(hypre_BoxArraySize(boxes));

#ifdef OVERLAP_COMM_COMP

   /* subtract `dept_boxes' from `boxes' */
   hypre_ForBoxI(i, boxes)
      {
         dept_box_a = hypre_BoxArrayArrayBoxArray(dept_boxes, i);

         /* initialize `indt_box_a' */
         indt_box_a = hypre_BoxArrayArrayBoxArray(indt_boxes, i);
         hypre_AppendBox(hypre_BoxArrayBox(boxes, i), indt_box_a);

         /* subtract `dept_box_a' from `indt_box_a' */
         hypre_ForBoxI(j, dept_box_a)
            {
               box_a0 = hypre_NewBoxArray(0);

               hypre_ForBoxI(k, indt_box_array)
                  {
                     box_a1 =
                        hypre_SubtractBoxes(hypre_BoxArrayBox(indt_box_a, k),
                                            hypre_BoxArrayBox(dept_box_a, j));
                     hypre_AppendBoxArray(box_a1, box_a0);
                     hypre_FreeBoxArray(box_a1);
                  }

               hypre_FreeBoxArray(indt_box_a);
               indt_box_a = box_a0;
            }

         /* union `indt_box_a' to minimize size of `indt_boxes' */
         hypre_BoxArrayArrayBoxArray(indt_boxes, i) =
            hypre_UnionBoxArray(indt_box_a);
      }

#else

#endif

   /*------------------------------------------------------
    * Return
    *------------------------------------------------------*/

   *send_boxes_ptr = send_boxes;
   *recv_boxes_ptr = recv_boxes;
   *send_processes_ptr = send_processes;
   *recv_processes_ptr = recv_processes;
   *indt_boxes_ptr = indt_boxes;
   *dept_boxes_ptr = dept_boxes;

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_NewComputePkg:
 *--------------------------------------------------------------------------*/

hypre_ComputePkg *
hypre_NewComputePkg( hypre_BoxArrayArray   *send_boxes,
                     hypre_BoxArrayArray   *recv_boxes,
                     hypre_Index            send_stride,
                     hypre_Index            recv_stride,
                     int                  **send_processes,
                     int                  **recv_processes,
                     hypre_BoxArrayArray   *indt_boxes,
                     hypre_BoxArrayArray   *dept_boxes,
                     hypre_Index            stride,
                     hypre_StructGrid      *grid,
                     hypre_BoxArray        *data_space,
                     int                    num_values     )
{
   hypre_ComputePkg  *compute_pkg;

   compute_pkg = hypre_CTAlloc(hypre_ComputePkg, 1);

   hypre_ComputePkgCommPkg(compute_pkg)     =
      hypre_NewCommPkg(send_boxes, recv_boxes,
                       send_stride, recv_stride,
                       data_space, data_space,
                       send_processes, recv_processes,
                       num_values, hypre_StructGridComm(grid));

   hypre_ComputePkgIndtBoxes(compute_pkg)   = indt_boxes;
   hypre_ComputePkgDeptBoxes(compute_pkg)   = dept_boxes;
   hypre_CopyIndex(stride, hypre_ComputePkgStride(compute_pkg));

   hypre_ComputePkgGrid(compute_pkg)        = grid;
   hypre_ComputePkgDataSpace(compute_pkg)   = data_space;
   hypre_ComputePkgNumValues(compute_pkg)   = num_values;

   return compute_pkg;
}

/*--------------------------------------------------------------------------
 * hypre_FreeComputePkg:
 *--------------------------------------------------------------------------*/

int
hypre_FreeComputePkg( hypre_ComputePkg *compute_pkg )
{
   int ierr = 0;

   if (compute_pkg)
   {
      hypre_FreeCommPkg(hypre_ComputePkgCommPkg(compute_pkg));

      hypre_FreeBoxArrayArray(hypre_ComputePkgIndtBoxes(compute_pkg));
      hypre_FreeBoxArrayArray(hypre_ComputePkgDeptBoxes(compute_pkg));

      hypre_TFree(compute_pkg);
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_InitializeIndtComputations:
 *--------------------------------------------------------------------------*/

hypre_CommHandle *
hypre_InitializeIndtComputations( hypre_ComputePkg *compute_pkg,
                                  double           *data        )
{
   hypre_CommPkg *comm_pkg = hypre_ComputePkgCommPkg(compute_pkg);

   return hypre_InitializeCommunication(comm_pkg, data, data);
}

/*--------------------------------------------------------------------------
 * hypre_FinalizeIndtComputations:
 *--------------------------------------------------------------------------*/

int
hypre_FinalizeIndtComputations( hypre_CommHandle *comm_handle )
{
   int ierr = 0;

   hypre_FinalizeCommunication(comm_handle);

   return ierr;
}
