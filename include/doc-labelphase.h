
/*!
  \defgroup labeling Labeling Phase

  Introduction
  ============

  The labeling phase is the first step in order to process the image and extract the necessary statistics for
  the star identification process. Nevertheless some very simple preprocessing is done before the actual labeling
  takes place. In total the labeling phase consists of 3 stages:
    1. Thresholding and initial labeling
    2. Merging with the highest label using a forward or backward mask
    3. Consolidation

  Stage 1 is only done once whereas stages 2 and 3 are done in an iteratitve manner.

   Thresholding and initial labeling
   =================================

   The connected component labeling algorithms is originally designed to work with binary images.
   However for the computation of the weighted centroid of each star spot
   the illumination values of the pixels have to be considered as well. Therefore a thresholding
   operation is applied to all pixels setting all pixels below the threshold to the background color 0,
   thereby effectively creating the binary image for the algorithm (zero and non-zero pixels) while
   preserving the illumination information of the non-zero pixels. Figure 1 shows the operation exemplary.

    @image html original-thresholding.svg "Figure 1 Thresholding operation performed on the original image"

   Additionally all non-zero pixels are assigned an initial label. This initial label consists of their
   x and y pixel coordinates incremented by one. This is because the label [0,0] is reserverd for the
   background color. During this assignment non-zero-pixels whose 8 neighboring pixels are zero-pixels
   are set to zero too. This filters out the numerous "hot pixels" in the beginning. Figure 2 shows the r
   result of this stage.

   @image html initial-labeling.svg "Figure 2 Filtering of single hot pixels and initial labeling"

   Iterative labeling
   ==================

   In the merging stage every non-zero pixel samples a forward mask (i.e sample of all neighbor pixels with higher pixel coordinates) or
   a backward mask (i.e. sample of all neighbor pixels with smaller pixels coordinates), determines the highest label
   among the 5 pixels and assigns it to itself. In figure 3 the forward and backwards mask are depicted graphically.

   @image html forward-backward.svg "Figure 3 Forward and backward mask (light blue) of a certain pixel (dark blue)"

   Finally in the consolidation stage each non-zero pixel reads the value of the pixel its label points to and assigns this value to itself.
   Figure 4 shows the merging with a forward mask and the consolidation stage applied to the example image.

   @image html labeling.svg "Figure 4 Merging stage with forward mask (middle) followed by a consolidation stage (right)"

   This process is repeated several times while alternating between forward and backwards mask. This algorithm is quite efficient
   for spots with elliptical shape as they are expected in a star image. However, it proved to be not very efficient as a generic
   connected component labeling algorithm.


*/
