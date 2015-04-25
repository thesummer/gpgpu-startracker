
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

   The counting stage is now responsible to count 2 metrics for each star spot:

    - The number of pixels
    - The sum of the accumulated luminance values of all pixels of the star spot

    Each pixel is represented by 4 bytes. In the current version these 4 bytes are used to store 2 unsigned short integer numbers,
    one for storing the number of pixels and one for storing the luminance sum.
    Therefore in the worst case only 256 pixels with a maximum luminance value of 255 could be stored. As star spots are assumed to consists of
    a comparably small number of pixels and only few pixels will reach high values of luminance this boundary does not pose a problem.

    The counting stage works in theory like a 2-dimensional version of the running some of the \ref reduction. It starts with an initial pass
    where each pixel belonging to a label is assigned the values (1, luminance), i.e. 1 as an initial count of pixels belonging to the star spot
    of the corresponding label and the luminance value of this pixel as it appeared in the original image. The left image of Figure 2 shows this
    initilalization for the pixel counts.

    In subsequent passes each pixel which is part of a filled area resulting from the filling stage adds the values of the three corner pixels
    (x-2^n, y), (x, y-2^n), (x-2^n, y-2^n) (with n being the number of the current pass) to its own values. In the end the root pixel will have
    stored the accumulated sum of a 2^n*2^n square. Figure 2 shows the algorithm in practice for the pixel counts. In the left image the pixels
    identified in the \ref labeling are initialized to 1. In the middle image each pixel of the filled area from the filling stage now contains
    the number of labeled pixels in a 2-by-2 square with each pixel being the top-right corner (exemplary illustrated for the root pixel).
    Finally in the right image each pixel contains the number of labeled pixels in a 4-by-4 square, hence the root pixel now has the full number
    of pixels for this label.

    @image html stats-countstage.svg "Figure 2 Counting stage: Left initialization, Middle after iteration 1, Right: after iteration 2"

    The results of this stage are then copied to the texture holding the results of the \ref reduction, but with an offset of a few columns.

   Centroiding stage
   =================

    The centroiding stage operates the same way as the counting stage, but collects different data.
    For the weighted centroiding the sum is created after the following formula:

    s_x = sum(x_i * p_i)

    s_y = sum(y_i * p_i)

    with x_i and y_i being the relative position of the the ith pixel of a label with respect to the root pixel and with p_i being the luminance value of the ith pixel.
    However, due to the multiplication, the sum could easily exceed the 16bit integer range used in the counting stage. Therefore 3 bytes are used per pixel to allow for 24 bit integers
    with the downside that the centroiding stage has to be executed twice, once for s_x and once for s_y.

    The results of both runs are then stored in the result of the \ref reduction in a similar manner as in the counting stage, but with different offsets.
    An impression of the final combined results of all stages is shown in Figure 3 for the intial example of \ref reduction. In the bottom-left area results of the \ref reduction act as a 2d-list
    of all extracted star spots a few columns to the left the results of the counting stage are aligned in the same layout, follwing by the results of the
    computation of the centroiding sum in x- and y-direction. In real examples the area necessary to download from the graphics memory in order to use
    the statistics for further processing will be much smaller than the whole image as it would only be a small area around the lower left corner of the image.

    @image html stats-results.svg "Figure 3 Final results of the Stats Phase: Left Initial Image from the \ref labeling, Right: Layout of the statistics result"

*/
