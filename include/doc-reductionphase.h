
/*!
  \defgroup reduction Reduction Phase

  Introduction
  ============

  The input of the reduction phase is the results of the \ref labeling, i.e. an image containing
  a set of disjoint connected areas. Pixels which belong to the same connected area have a label
  assigned to them which corresponds to the x-y-coordinates of the root-pixel of the
  connected area.

  The goal of the reduction phase is to create a 2D-list of all identified connected areas at
  the top-left corner of the image.
  The Reduction Phase follows the reduction algorithm described in <a href="http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter36.html">Nvidia GPU Gems 2</a>
  to a large extend. The reduction if first executed horizontally and then vertically.
  It is split into 3 stages:
    1. Root-pixel filtering
    2. Running sum for all zero pixel
    3. Binary search and reordering of the pixels

  Root-pixel filtering
  ==============

  The initalization is comparably simple. For the reduction operation the size and shape of
  each identified connected area is not important, but only the label i.e. the coordinates of
  the root pixels are of importance. Therefore the initialization stage will simply set the value
  of all pixels which are not root-pixels (i.e. pixels whose stored values
  equal their x-y-coordinates) to zero. Figure 1 gives a simple overview of it.

  @image html reduction-root.svg "Figure 1 Set all non-root pixels to zero"


  Running sum for all zero pixels
  ===============================

  In a first step initial values for each pixel are stored after the following scheme:

   - If the pixel is zero (not a root pixel), it is assigned the value 1, otherwise 0
   - If the pixel to the left is zero the value is increased by 1

   Figure 2 presents the initialization step.

  @image html reduction-init.svg "Figure 2 Initialize pixel values for running sum"

  Is the following iteration each pixel adds the value of the pixel 2^i to the left.
  i is the count of the current iteration (starting with 1). Figure 3 gives an overview
  of this process:

  @image html reduction-sum.svg "Figure 3 Computing the sum for each pixel horizontally"

  Binary search and reordering of pixels
  ======================================

  After the running sum each pixel has the information where it has to move in order
  to close the gaps. However, it only has write access to itself. Therefore each destination
  pixel has to find its corresponding source pixel and copy its value.
  Because the running sum is monotonic ascending for each row, a binary search can be applied.
  Each pixel searches for a pixel to its right where the sum is equal to the distance between
  the two. If the sum is larger than the inter-pixel distance then the pixel has to search for
  its source pixel further to the right in the next iteration. If the sum is smaller than the
  distance the destination pixel has to search further left for the source pixel in the next
  iteration. The step size is divided by two for each iteration until it reaches one. Then
  each destination pixel either has found a source pixel and copies its data or it has not
  found a source pixel and is set to zero.

  Figure 4 shows the result of the reduction operation. First the input image is reduced
  horizontally, then vertically.

  @image html reduction-result.svg "Figure 4 Result after horizontal (left) and vertical (right) reduction"

*/
