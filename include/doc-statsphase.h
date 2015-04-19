
/*!
  \defgroup stats Statistics Phase

  Introduction
  ============

  The statistics phase is the last phase of image processing. The goal is to use the information extracted in
  the previous 2 phases to compute the statistics for each star spot, namely the pixel count and the
  weighted sums for centroiding computation. The phase is divided into 3 stages:

   Filling stage
   =============

   The filling stage is necessary to account for possible holes in the labeled star spot. It will iteratively fill
   a an area 2^n pixels around every star spot with the root pixel as one corner. The main reason for the need of this
   stage is again that it is not possible to have scatter (random write) operation on the GPU. In the counting stage
   the pixels of a spot are counted in a similar fashion as the zero-pixels in the reduction phase. However, pixels which
   on an edge or in a whole within the star spot miss the information if they should sum up the pixels of a certain label
   in their proximity. With the filled area of this stage, all pixels therein "know" that they will need to contribute to
   the counting of other pixels of a certain label, even if they themselves belong to the background.
   In figure 1 the filling process is shown as an example. The first image shows the labeled spot with its root pixel being
   the most top-right pixel of the spot.
   In the first iteration (middle image) every zero-pixel samples the 3 pixels (x+1,y), (x, y+1) and (x+1, y+1) from its current
   position. In case one or more of these pixels have a label, it computes the distance between its position and the root pixel
   of the label(s) and assigns the one with the smalles distance to itself. In the second iteration every zero-pixel samples the
   corner pixel of a 2^n*2^n square with itself being the bottom left corner, i.e. for the 2nd pass the pixels (x+2,y), (y+2), and
   (x+2, y+2) and follows the algorithm explained above. This is shown in the right image in figure 1.
   With multiple star spots in the same area or after a certain number of iterations the filled areas between 2 (or more) spots may
   directly meet, but does not pose any problem as the filled area only marks a maximum area around a star spot and is guaranteed to be
   at least as big as the star spot itself.

   @image html stats-fillstage.svg "Figure 1 Filling stage"

   Counting stage
   ==============



   Centroiding stage
   =================

*/
