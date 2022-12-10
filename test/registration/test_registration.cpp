/*
* Software License Agreement (BSD License)
*
*  Point Cloud Library (PCL) - www.pointclouds.org
*  Copyright (c) 2010-2011, Willow Garage, Inc.
*
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the copyright holder(s) nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
* $Id$
*
*/

#include <pcl/test/gtest.h>

#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/features/normal_3d.h>
#include <pcl/registration/registration.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/joint_icp.h>
#include <pcl/registration/icp_nl.h>
#include <pcl/registration/gicp.h>
#include <pcl/registration/gicp6d.h>
#include <pcl/registration/transformation_estimation_point_to_plane.h>
#include <pcl/registration/transformation_validation_euclidean.h>
#include <pcl/registration/correspondence_rejection_median_distance.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>
#include <pcl/registration/correspondence_rejection_surface_normal.h>
#include <pcl/registration/correspondence_estimation_normal_shooting.h>
#include <pcl/registration/pyramid_feature_matching.h>
#include <pcl/features/ppf.h>
#include <pcl/registration/ppf_registration.h>
#include <pcl/filters/voxel_grid.h>
// We need Histogram<2> to function, so we'll explicitly add kdtree_flann.hpp here
#include <pcl/kdtree/impl/kdtree_flann.hpp>
//(pcl::Histogram<2>)

using namespace pcl;
using namespace pcl::io;

PointCloud<PointXYZ> cloud_source, cloud_target, cloud_reg;
PointCloud<PointXYZRGBA> cloud_with_color;

template <typename PointSource, typename PointTarget>
class RegistrationWrapper : public Registration<PointSource, PointTarget>
{
public:
  void computeTransformation (pcl::PointCloud<PointSource> &, const Eigen::Matrix4f&) override { }

  bool hasValidFeaturesTest ()
  {
    return (this->hasValidFeatures ());
  }
  void findFeatureCorrespondencesTest (int index, std::vector<int> &correspondence_indices)
  {
    this->findFeatureCorrespondences (index, correspondence_indices);
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, findFeatureCorrespondences)
{
  using FeatureT = Histogram<2>;
  using FeatureCloud = PointCloud<FeatureT>;

  RegistrationWrapper <PointXYZ, PointXYZ> reg;

  FeatureCloud feature0, feature1, feature2, feature3;
  feature0.height = feature1.height = feature2.height = feature3.height = 1;
  feature0.is_dense = feature1.is_dense = feature2.is_dense = feature3.is_dense = true;

  for (float x = -5.0f; x <= 5.0f; x += 0.2f)
  {
    for (float y = -5.0f; y <= 5.0f; y += 0.2f)
    {
      FeatureT f;
      f.histogram[0] = x;
      f.histogram[1] = y;
      feature0.push_back (f);

      f.histogram[0] = x;
      f.histogram[1] = y - 2.5f;
      feature1.push_back (f);

      f.histogram[0] = x - 2.0f;
      f.histogram[1] = y + 1.5f;
      feature2.push_back (f);

      f.histogram[0] = x + 2.0f;
      f.histogram[1] = y + 1.5f;
      feature3.push_back (f);
    }
  }
  feature0.width = feature0.size ();
  feature1.width = feature1.size ();
  feature2.width = feature2.size ();
  feature3.width = feature3.size ();

  KdTreeFLANN<FeatureT> tree;

/*  int k = 600;

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature1");
  reg.setTargetFeature<FeatureT> (feature1.makeShared (), "feature1");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature1");

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature2");
  reg.setTargetFeature<FeatureT> (feature2.makeShared (), "feature2");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature2");

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature3");
  reg.setTargetFeature<FeatureT> (feature3.makeShared (), "feature3");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature3");

  ASSERT_TRUE (reg.hasValidFeaturesTest ());

  std::vector<int> indices;
  reg.findFeatureCorrespondencesTest (1300, indices);

  ASSERT_EQ ((int)indices.size (), 10);
  const int correct_values[] = {1197, 1248, 1249, 1299, 1300, 1301, 1302, 1350, 1351, 1401};
  for (std::size_t i = 0; i < indices.size (); ++i)
  {
    EXPECT_EQ (indices[i], correct_values[i]);
  }
*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This test if the ICP algorithm can successfully find the transformation of a cloud that has been
// moved 0.7 in x direction.
// It indirectly test the KDTree doesn't get an empty input cloud, see #3624
// It is more or less a copy of https://github.com/PointCloudLibrary/pcl/blob/cc7fe363c6463a0abc617b1e17e94ab4bd4169ef/doc/tutorials/content/sources/iterative_closest_point/iterative_closest_point.cpp
TEST(PCL, ICP_translated)
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_in(new pcl::PointCloud<pcl::PointXYZ>(5,1));
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out(new pcl::PointCloud<pcl::PointXYZ>);

  // Fill in the CloudIn data
  for (auto& point : *cloud_in)
  {
    point.x = 1024 * rand() / (RAND_MAX + 1.0f);
    point.y = 1024 * rand() / (RAND_MAX + 1.0f);
    point.z = 1024 * rand() / (RAND_MAX + 1.0f);
  }

  *cloud_out = *cloud_in;

  for (auto& point : *cloud_out)
    point.x += 0.7f;

  pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;
  icp.setInputSource(cloud_in);
  icp.setInputTarget(cloud_out);

  pcl::PointCloud<pcl::PointXYZ> Final;
  icp.align(Final);

  // Check that we have successfully converged
  ASSERT_TRUE(icp.hasConverged());

  // Test that the fitness score is below acceptable threshold
  EXPECT_LT(icp.getFitnessScore(), 1e-6);

  // Ensure that the translation found is within acceptable threshold.
  EXPECT_NEAR(icp.getFinalTransformation()(0, 3), 0.7, 2e-3);
  EXPECT_NEAR(icp.getFinalTransformation()(1, 3), 0.0, 2e-3);
  EXPECT_NEAR(icp.getFinalTransformation()(2, 3), 0.0, 2e-3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPoint)
{
  IterativeClosestPoint<PointXYZ, PointXYZ> reg;
  PointCloud<PointXYZ>::ConstPtr source (cloud_source.makeShared ());
  reg.setInputSource (source);
  reg.setInputTarget (cloud_target.makeShared ());
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);
  reg.setMaxCorrespondenceDistance (0.05);

  // Register
  reg.align (cloud_reg);
  EXPECT_EQ (cloud_reg.size (), cloud_source.size ());

  Eigen::Matrix4f transformation = reg.getFinalTransformation ();
  EXPECT_NEAR (transformation (0, 0), 0.8806,  1e-3);
  EXPECT_NEAR (transformation (0, 1), 0.036481287330389023, 1e-2);
  EXPECT_NEAR (transformation (0, 2), -0.4724, 1e-3);
  EXPECT_NEAR (transformation (0, 3), 0.03453, 1e-3);

  EXPECT_NEAR (transformation (1, 0), -0.02354,  1e-3);
  EXPECT_NEAR (transformation (1, 1),  0.9992,   1e-3);
  EXPECT_NEAR (transformation (1, 2),  0.03326,  1e-3);
  EXPECT_NEAR (transformation (1, 3), -0.001519, 1e-3);

  EXPECT_NEAR (transformation (2, 0),  0.4732,  1e-3);
  EXPECT_NEAR (transformation (2, 1), -0.01817, 1e-3);
  EXPECT_NEAR (transformation (2, 2),  0.8808,  1e-3);
  EXPECT_NEAR (transformation (2, 3),  0.04116, 1e-3);

  EXPECT_EQ (transformation (3, 0), 0);
  EXPECT_EQ (transformation (3, 1), 0);
  EXPECT_EQ (transformation (3, 2), 0);
  EXPECT_EQ (transformation (3, 3), 1);
}

TEST (PCL, IterativeClosestPointWithNormals)
{
  IterativeClosestPointWithNormals<PointNormal, PointNormal, float> reg_float;
  reg_float.setUseSymmetricObjective(true);
  EXPECT_TRUE(reg_float.getUseSymmetricObjective());

  IterativeClosestPointWithNormals<PointNormal, PointNormal, double> reg_double;
  reg_double.setUseSymmetricObjective(true);
  EXPECT_TRUE(reg_double.getUseSymmetricObjective());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void
sampleRandomTransform (Eigen::Affine3f &trans, float max_angle, float max_trans)
{
    srand(0);
    // Sample random transform
    Eigen::Vector3f axis((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
    axis.normalize();
    float angle = (float)rand() / RAND_MAX * max_angle;
    Eigen::Vector3f translation((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
    translation *= max_trans;
    Eigen::Affine3f rotation(Eigen::AngleAxis<float>(angle, axis));
    trans = Eigen::Translation3f(translation) * rotation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPointWithRejectors)
{
  IterativeClosestPoint<PointXYZ, PointXYZ> reg;
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);
  reg.setMaxCorrespondenceDistance (0.15);
  // Add a median distance rejector
  pcl::registration::CorrespondenceRejectorMedianDistance::Ptr rej_med (new pcl::registration::CorrespondenceRejectorMedianDistance);
  rej_med->setMedianFactor (4.0);
  reg.addCorrespondenceRejector (rej_med);
  // Also add a SaC rejector
  pcl::registration::CorrespondenceRejectorSampleConsensus<PointXYZ>::Ptr rej_samp (new pcl::registration::CorrespondenceRejectorSampleConsensus<PointXYZ>);
  reg.addCorrespondenceRejector (rej_samp);

  std::size_t ntransforms = 10;
  for (std::size_t t = 0; t < ntransforms; t++)
  {
    // Sample a fixed offset between cloud pairs
    Eigen::Affine3f delta_transform;
    sampleRandomTransform (delta_transform, 0., 0.05);
    // Sample random global transform for each pair, to make sure we aren't biased around the origin
    Eigen::Affine3f net_transform;
    sampleRandomTransform (net_transform, 2*M_PI, 10.);

    PointCloud<PointXYZ>::ConstPtr source (cloud_source.makeShared ());
    PointCloud<PointXYZ>::Ptr source_trans (new PointCloud<PointXYZ>);
    PointCloud<PointXYZ>::Ptr target_trans (new PointCloud<PointXYZ>);

    pcl::transformPointCloud (*source, *source_trans, delta_transform.inverse () * net_transform);
    pcl::transformPointCloud (*source, *target_trans, net_transform);

    reg.setInputSource (source_trans);
    reg.setInputTarget (target_trans);

    // Register
    reg.align (cloud_reg);
    Eigen::Matrix4f trans_final = reg.getFinalTransformation ();
    // Translation should be within 1cm
    for (int y = 0; y < 4; y++)
      EXPECT_NEAR (trans_final (y, 3), delta_transform (y, 3), 1E-2);
    // Rotation within .1
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 3; x++)
        EXPECT_NEAR (trans_final (y, x), delta_transform (y, x), 1E-1);

  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, JointIterativeClosestPoint)
{
  // Set up
  JointIterativeClosestPoint<PointXYZ, PointXYZ> reg;
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);
  reg.setMaxCorrespondenceDistance (0.25); // Making sure the correspondence distance > the max translation
  // Add a median distance rejector
  pcl::registration::CorrespondenceRejectorMedianDistance::Ptr rej_med (new pcl::registration::CorrespondenceRejectorMedianDistance);
  rej_med->setMedianFactor (8.0);
  reg.addCorrespondenceRejector (rej_med);
  // Also add a SaC rejector
  pcl::registration::CorrespondenceRejectorSampleConsensus<PointXYZ>::Ptr rej_samp (new pcl::registration::CorrespondenceRejectorSampleConsensus<PointXYZ>);
  reg.addCorrespondenceRejector (rej_samp);

  std::size_t ntransforms = 10;
  for (std::size_t t = 0; t < ntransforms; t++)
  {

    // Sample a fixed offset between cloud pairs
    Eigen::Affine3f delta_transform;
    // No rotation, since at a random offset this could make it converge to a wrong (but still reasonable) result
    sampleRandomTransform (delta_transform, 0., 0.10);
    // Make a few transformed versions of the data, plus noise
    std::size_t nclouds = 5;
    for (std::size_t i = 0; i < nclouds; i++)
    {
      PointCloud<PointXYZ>::ConstPtr source (cloud_source.makeShared ());
      // Sample random global transform for each pair
      Eigen::Affine3f net_transform;
      sampleRandomTransform (net_transform, 2*M_PI, 10.);
      // And apply it to the source and target
      PointCloud<PointXYZ>::Ptr source_trans (new PointCloud<PointXYZ>);
      PointCloud<PointXYZ>::Ptr target_trans (new PointCloud<PointXYZ>);
      pcl::transformPointCloud (*source, *source_trans, delta_transform.inverse () * net_transform);
      pcl::transformPointCloud (*source, *target_trans, net_transform);
      // Add these to the joint solver
      reg.addInputSource (source_trans);
      reg.addInputTarget (target_trans);

    }

    // Register
    reg.align (cloud_reg);
    Eigen::Matrix4f trans_final = reg.getFinalTransformation ();
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++)
        EXPECT_NEAR (trans_final (y, x), delta_transform (y, x), 1E-2);

    EXPECT_TRUE (cloud_reg.empty ()); // By definition, returns an empty cloud
    // Clear
    reg.clearInputSources ();
    reg.clearInputTargets ();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPointNonLinear)
{
  using PointT = PointXYZRGB;
  PointCloud<PointT>::Ptr temp_src (new PointCloud<PointT>);
  copyPointCloud (cloud_source, *temp_src);
  PointCloud<PointT>::Ptr temp_tgt (new PointCloud<PointT>);
  copyPointCloud (cloud_target, *temp_tgt);
  PointCloud<PointT> output;

  IterativeClosestPointNonLinear<PointT, PointT> reg;
  reg.setInputSource (temp_src);
  reg.setInputTarget (temp_tgt);
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);

  // Register
  reg.align (output);
  EXPECT_EQ (output.size (), cloud_source.size ());
  // We get different results on 32 vs 64-bit systems.  To address this, we've removed the explicit output test
  // on the transformation matrix.  Instead, we're testing to make sure the algorithm converges to a sufficiently
  // low error by checking the fitness score.
  /*
  Eigen::Matrix4f transformation = reg.getFinalTransformation ();

  EXPECT_NEAR (transformation (0, 0),  0.941755, 1e-2);
  EXPECT_NEAR (transformation (0, 1),  0.147362, 1e-2);
  EXPECT_NEAR (transformation (0, 2), -0.281285, 1e-2);
  EXPECT_NEAR (transformation (0, 3),  0.029813, 1e-2);

  EXPECT_NEAR (transformation (1, 0), -0.111655, 1e-2);
  EXPECT_NEAR (transformation (1, 1),  0.990408, 1e-2);
  EXPECT_NEAR (transformation (1, 2),  0.139090, 1e-2);
  EXPECT_NEAR (transformation (1, 3), -0.001864, 1e-2);

  EXPECT_NEAR (transformation (2, 0),  0.297271, 1e-2);
  EXPECT_NEAR (transformation (2, 1), -0.092015, 1e-2);
  EXPECT_NEAR (transformation (2, 2),  0.939670, 1e-2);
  EXPECT_NEAR (transformation (2, 3),  0.042721, 1e-2);

  EXPECT_EQ (transformation (3, 0), 0);
  EXPECT_EQ (transformation (3, 1), 0);
  EXPECT_EQ (transformation (3, 2), 0);
  EXPECT_EQ (transformation (3, 3), 1);
  */
  EXPECT_LT (reg.getFitnessScore (), 0.001);

  // Check again, for all possible caching schemes
  for (int iter = 0; iter < 4; iter++)
  {
    bool force_cache = static_cast<bool> (iter / 2);
    bool force_cache_reciprocal = static_cast<bool> (iter % 2);
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    // Ensure that, when force_cache is not set, we are robust to the wrong input
    if (force_cache)
      tree->setInputCloud (temp_tgt);
    reg.setSearchMethodTarget (tree, force_cache);

    pcl::search::KdTree<PointT>::Ptr tree_recip (new pcl::search::KdTree<PointT>);
    if (force_cache_reciprocal)
      tree_recip->setInputCloud (temp_src);
    reg.setSearchMethodSource (tree_recip, force_cache_reciprocal);

    // Register
    reg.align (output);
    EXPECT_EQ (output.size (), cloud_source.size ());
    EXPECT_LT (reg.getFitnessScore (), 0.001);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPoint_PointToPlane)
{
  using PointT = PointNormal;
  PointCloud<PointT>::Ptr src (new PointCloud<PointT>);
  copyPointCloud (cloud_source, *src);
  PointCloud<PointT>::Ptr tgt (new PointCloud<PointT>);
  copyPointCloud (cloud_target, *tgt);
  PointCloud<PointT> output;

  NormalEstimation<PointNormal, PointNormal> norm_est;
  norm_est.setSearchMethod (search::KdTree<PointNormal>::Ptr (new search::KdTree<PointNormal>));
  norm_est.setKSearch (10);
  norm_est.setInputCloud (tgt);
  norm_est.compute (*tgt);

  IterativeClosestPoint<PointT, PointT> reg;
  using PointToPlane = registration::TransformationEstimationPointToPlane<PointT, PointT>;
  PointToPlane::Ptr point_to_plane (new PointToPlane);
  reg.setTransformationEstimation (point_to_plane);
  reg.setInputSource (src);
  reg.setInputTarget (tgt);
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);
  // Use a correspondence estimator which needs normals
  registration::CorrespondenceEstimationNormalShooting<PointT, PointT, PointT>::Ptr ce (new registration::CorrespondenceEstimationNormalShooting<PointT, PointT, PointT>);
  reg.setCorrespondenceEstimation (ce);
  // Add rejector
  registration::CorrespondenceRejectorSurfaceNormal::Ptr rej (new registration::CorrespondenceRejectorSurfaceNormal);
  rej->setThreshold (0); //Could be a lot of rotation -- just make sure they're at least within 0 degrees
  reg.addCorrespondenceRejector (rej);

  // Register
  reg.align (output);
  EXPECT_EQ (output.size (), cloud_source.size ());
  EXPECT_LT (reg.getFitnessScore (), 0.005);

  // Check again, for all possible caching schemes
  for (int iter = 0; iter < 4; iter++)
  {
    bool force_cache = static_cast<bool> (iter/2);
    bool force_cache_reciprocal = static_cast<bool> (iter%2);
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    // Ensure that, when force_cache is not set, we are robust to the wrong input
    if (force_cache)
      tree->setInputCloud (tgt);
    reg.setSearchMethodTarget (tree, force_cache);

    pcl::search::KdTree<PointT>::Ptr tree_recip (new pcl::search::KdTree<PointT>);
    if (force_cache_reciprocal)
      tree_recip->setInputCloud (src);
    reg.setSearchMethodSource (tree_recip, force_cache_reciprocal);

    // Register
    reg.align (output);
    EXPECT_EQ (output.size (), cloud_source.size ());
    EXPECT_LT (reg.getFitnessScore (), 0.005);
  }



  // We get different results on 32 vs 64-bit systems.  To address this, we've removed the explicit output test
  // on the transformation matrix.  Instead, we're testing to make sure the algorithm converges to a sufficiently
  // low error by checking the fitness score.
  /*
  Eigen::Matrix4f transformation = reg.getFinalTransformation ();

  EXPECT_NEAR (transformation (0, 0),  0.9046, 1e-2);
  EXPECT_NEAR (transformation (0, 1),  0.0609, 1e-2);
  EXPECT_NEAR (transformation (0, 2), -0.4219, 1e-2);
  EXPECT_NEAR (transformation (0, 3),  0.0327, 1e-2);

  EXPECT_NEAR (transformation (1, 0), -0.0328, 1e-2);
  EXPECT_NEAR (transformation (1, 1),  0.9968, 1e-2);
  EXPECT_NEAR (transformation (1, 2),  0.0851, 1e-2);
  EXPECT_NEAR (transformation (1, 3), -0.0003, 1e-2);

  EXPECT_NEAR (transformation (2, 0),  0.4250, 1e-2);
  EXPECT_NEAR (transformation (2, 1), -0.0641, 1e-2);
  EXPECT_NEAR (transformation (2, 2),  0.9037, 1e-2);
  EXPECT_NEAR (transformation (2, 3),  0.0413, 1e-2);

  EXPECT_EQ (transformation (3, 0), 0);
  EXPECT_EQ (transformation (3, 1), 0);
  EXPECT_EQ (transformation (3, 2), 0);
  EXPECT_EQ (transformation (3, 3), 1);
  */
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, GeneralizedIterativeClosestPoint)
{
  using PointT = PointXYZ;
  PointCloud<PointT>::Ptr src (new PointCloud<PointT>);
  copyPointCloud (cloud_source, *src);
  PointCloud<PointT>::Ptr tgt (new PointCloud<PointT>);
  copyPointCloud (cloud_target, *tgt);
  PointCloud<PointT> output;

  GeneralizedIterativeClosestPoint<PointT, PointT> reg;
  reg.setInputSource (src);
  reg.setInputTarget (tgt);
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);

  // Register
  reg.align (output);
  EXPECT_EQ (output.size (), cloud_source.size ());
  EXPECT_LT (reg.getFitnessScore (), 0.0001);

  // Check again, for all possible caching schemes
  for (int iter = 0; iter < 4; iter++)
  {
    bool force_cache = static_cast<bool> (iter/2);
    bool force_cache_reciprocal = static_cast<bool> (iter%2);
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    // Ensure that, when force_cache is not set, we are robust to the wrong input
    if (force_cache)
      tree->setInputCloud (tgt);
    reg.setSearchMethodTarget (tree, force_cache);

    pcl::search::KdTree<PointT>::Ptr tree_recip (new pcl::search::KdTree<PointT>);
    if (force_cache_reciprocal)
      tree_recip->setInputCloud (src);
    reg.setSearchMethodSource (tree_recip, force_cache_reciprocal);

    // Register
    reg.align (output);
    EXPECT_EQ (output.size (), cloud_source.size ());
    EXPECT_LT (reg.getFitnessScore (), 0.001);
  }

  // Test guess matrix
  Eigen::Isometry3f transform = Eigen::Isometry3f (Eigen::AngleAxisf (0.25 * M_PI, Eigen::Vector3f::UnitX ())
                                                 * Eigen::AngleAxisf (0.50 * M_PI, Eigen::Vector3f::UnitY ())
                                                 * Eigen::AngleAxisf (0.33 * M_PI, Eigen::Vector3f::UnitZ ()));
  transform.translation () = Eigen::Vector3f (0.1, 0.2, 0.3);
  PointCloud<PointT>::Ptr transformed_tgt (new PointCloud<PointT>);
  pcl::transformPointCloud (*tgt, *transformed_tgt, transform.matrix ()); // transformed_tgt is now a copy of tgt with a transformation matrix applied

  GeneralizedIterativeClosestPoint<PointT, PointT> reg_guess;
  reg_guess.setInputSource (src);
  reg_guess.setInputTarget (transformed_tgt);
  reg_guess.setMaximumIterations (50);
  reg_guess.setTransformationEpsilon (1e-8);
  reg_guess.align (output, transform.matrix ());
  EXPECT_EQ (output.size (), cloud_source.size ());
  EXPECT_LT (reg.getFitnessScore (), 0.0001);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, GeneralizedIterativeClosestPoint6D)
{
  using PointT = PointXYZRGBA;
  Eigen::Affine3f delta_transform;
  PointCloud<PointT>::Ptr src_full (new PointCloud<PointT>);
  copyPointCloud (cloud_with_color, *src_full);
  PointCloud<PointT>::Ptr tgt_full (new PointCloud<PointT>);
  sampleRandomTransform (delta_transform, M_PI/0.1, .03);
  pcl::transformPointCloud (cloud_with_color, *tgt_full, delta_transform);
  PointCloud<PointT> output;

  // VoxelGrid filter
  PointCloud<PointT>::Ptr src (new PointCloud<PointT>);
  PointCloud<PointT>::Ptr tgt (new PointCloud<PointT>);
  pcl::VoxelGrid<PointT> sor;
  sor.setLeafSize (0.02f, 0.02f, 0.02f);
  sor.setInputCloud (src_full);
  sor.filter (*src);
  sor.setInputCloud (tgt_full);
  sor.filter (*tgt);

  GeneralizedIterativeClosestPoint6D reg;
  reg.setInputSource (src);
  reg.setInputTarget (tgt);
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);

  // Register
  reg.align (output);
  EXPECT_EQ (output.size (), src->size ());
  EXPECT_LT (reg.getFitnessScore (), 0.003);

  // Check again, for all possible caching schemes
  for (int iter = 0; iter < 4; iter++)
  {
    bool force_cache = static_cast<bool> (iter/2);
    bool force_cache_reciprocal = static_cast<bool> (iter%2);
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    // Ensure that, when force_cache is not set, we are robust to the wrong input
    if (force_cache)
      tree->setInputCloud (tgt);
    reg.setSearchMethodTarget (tree, force_cache);

    pcl::search::KdTree<PointT>::Ptr tree_recip (new pcl::search::KdTree<PointT>);
    if (force_cache_reciprocal)
      tree_recip->setInputCloud (src);
    reg.setSearchMethodSource (tree_recip, force_cache_reciprocal);

    // Register
    reg.align (output);
    EXPECT_EQ (output.size (), src->size ());
    EXPECT_LT (reg.getFitnessScore (), 0.003);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PyramidFeatureHistogram)
{
  // Create shared pointers
   PointCloud<PointXYZ>::Ptr cloud_source_ptr = cloud_source.makeShared (),
       cloud_target_ptr = cloud_target.makeShared ();

  PointCloud<Normal>::Ptr cloud_source_normals (new PointCloud<Normal> ()),
      cloud_target_normals (new PointCloud<Normal> ());
  search::KdTree<PointXYZ>::Ptr tree (new search::KdTree<PointXYZ>);
  NormalEstimation<PointXYZ, Normal> normal_estimator;
  normal_estimator.setSearchMethod (tree);
  normal_estimator.setRadiusSearch (0.05);
  normal_estimator.setInputCloud (cloud_source_ptr);
  normal_estimator.compute (*cloud_source_normals);

  normal_estimator.setInputCloud (cloud_target_ptr);
  normal_estimator.compute (*cloud_target_normals);


  PointCloud<PPFSignature>::Ptr ppf_signature_source (new PointCloud<PPFSignature> ()),
      ppf_signature_target (new PointCloud<PPFSignature> ());
  PPFEstimation<PointXYZ, Normal, PPFSignature> ppf_estimator;
  ppf_estimator.setInputCloud (cloud_source_ptr);
  ppf_estimator.setInputNormals (cloud_source_normals);
  ppf_estimator.compute (*ppf_signature_source);

  ppf_estimator.setInputCloud (cloud_target_ptr);
  ppf_estimator.setInputNormals (cloud_target_normals);
  ppf_estimator.compute (*ppf_signature_target);


  std::vector<std::pair<float, float> > dim_range_input, dim_range_target;
  for (std::size_t i = 0; i < 3; ++i) dim_range_input.emplace_back(static_cast<float> (-M_PI), static_cast<float> (M_PI));
  dim_range_input.emplace_back(0.0f, 1.0f);
  for (std::size_t i = 0; i < 3; ++i) dim_range_target.emplace_back(static_cast<float> (-M_PI) * 10.0f, static_cast<float> (M_PI) * 10.0f);
  dim_range_target.emplace_back(0.0f, 50.0f);


  PyramidFeatureHistogram<PPFSignature>::Ptr pyramid_source (new PyramidFeatureHistogram<PPFSignature> ()),
      pyramid_target (new PyramidFeatureHistogram<PPFSignature> ());
  pyramid_source->setInputCloud (ppf_signature_source);
  pyramid_source->setInputDimensionRange (dim_range_input);
  pyramid_source->setTargetDimensionRange (dim_range_target);
  pyramid_source->compute ();

  pyramid_target->setInputCloud (ppf_signature_target);
  pyramid_target->setInputDimensionRange (dim_range_input);
  pyramid_target->setTargetDimensionRange (dim_range_target);
  pyramid_target->compute ();

  float similarity_value = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value, 0.738492727, 1e-4);

  std::vector<std::pair<float, float> > dim_range_target2;
  for (std::size_t i = 0; i < 3; ++i) dim_range_target2.emplace_back(static_cast<float> (-M_PI) * 5.0f, static_cast<float> (M_PI) * 5.0f);
    dim_range_target2.emplace_back(0.0f, 20.0f);

  pyramid_source->setTargetDimensionRange (dim_range_target2);
  pyramid_source->compute ();

  pyramid_target->setTargetDimensionRange (dim_range_target2);
  pyramid_target->compute ();

  float similarity_value2 = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value2, 0.798465133, 1e-4);


  std::vector<std::pair<float, float> > dim_range_target3;
  for (std::size_t i = 0; i < 3; ++i) dim_range_target3.emplace_back(static_cast<float> (-M_PI) * 2.0f, static_cast<float> (M_PI) * 2.0f);
  dim_range_target3.emplace_back(0.0f, 10.0f);

  pyramid_source->setTargetDimensionRange (dim_range_target3);
  pyramid_source->compute ();

  pyramid_target->setTargetDimensionRange (dim_range_target3);
  pyramid_target->compute ();

  float similarity_value3 = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value3, 0.873699546, 1e-3);
}

// Suat G: disabled, since the transformation does not look correct.
// ToDo: update transformation from the ground truth.
#if 0
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PPFRegistration)
{
  // Transform the source cloud by a large amount
  Eigen::Vector3f initial_offset (100, 0, 0);
  float angle = M_PI/6;
  Eigen::Quaternionf initial_rotation (std::cos (angle / 2), 0, 0, sin (angle / 2));
  PointCloud<PointXYZ> cloud_source_transformed;
  transformPointCloud (cloud_source, cloud_source_transformed, initial_offset, initial_rotation);


  // Create shared pointers
  PointCloud<PointXYZ>::Ptr cloud_source_ptr, cloud_target_ptr, cloud_source_transformed_ptr;
  cloud_source_transformed_ptr = cloud_source_transformed.makeShared ();
  cloud_target_ptr = cloud_target.makeShared ();

  // Estimate normals for both clouds
  NormalEstimation<PointXYZ, Normal> normal_estimation;
  search::KdTree<PointXYZ>::Ptr search_tree (new search::KdTree<PointXYZ> ());
  normal_estimation.setSearchMethod (search_tree);
  normal_estimation.setRadiusSearch (0.05);
  PointCloud<Normal>::Ptr normals_target (new PointCloud<Normal> ()),
      normals_source_transformed (new PointCloud<Normal> ());
  normal_estimation.setInputCloud (cloud_target_ptr);
  normal_estimation.compute (*normals_target);
  normal_estimation.setInputCloud (cloud_source_transformed_ptr);
  normal_estimation.compute (*normals_source_transformed);

  PointCloud<PointNormal>::Ptr cloud_target_with_normals (new PointCloud<PointNormal> ()),
      cloud_source_transformed_with_normals (new PointCloud<PointNormal> ());
  concatenateFields (*cloud_target_ptr, *normals_target, *cloud_target_with_normals);
  concatenateFields (*cloud_source_transformed_ptr, *normals_source_transformed, *cloud_source_transformed_with_normals);

  // Compute PPFSignature feature clouds for source cloud
  PPFEstimation<PointXYZ, Normal, PPFSignature> ppf_estimator;
  PointCloud<PPFSignature>::Ptr features_source_transformed (new PointCloud<PPFSignature> ());
  ppf_estimator.setInputCloud (cloud_source_transformed_ptr);
  ppf_estimator.setInputNormals (normals_source_transformed);
  ppf_estimator.compute (*features_source_transformed);


  // Train the source cloud - create the hash map search structure
  PPFHashMapSearch::Ptr hash_map_search (new PPFHashMapSearch (15.0 / 180 * M_PI,
                                                               0.05));
  hash_map_search->setInputFeatureCloud (features_source_transformed);

  // Finally, do the registration
  PPFRegistration<PointNormal, PointNormal> ppf_registration;
  ppf_registration.setSceneReferencePointSamplingRate (20);
  ppf_registration.setPositionClusteringThreshold (0.15);
  ppf_registration.setRotationClusteringThreshold (45.0 / 180 * M_PI);
  ppf_registration.setSearchMethod (hash_map_search);
  ppf_registration.setInputCloud (cloud_source_transformed_with_normals);
  ppf_registration.setInputTarget (cloud_target_with_normals);

  PointCloud<PointNormal> cloud_output;
  ppf_registration.align (cloud_output);
  Eigen::Matrix4f transformation = ppf_registration.getFinalTransformation ();

  EXPECT_NEAR (transformation(0, 0), -0.153768, 1e-4);
  EXPECT_NEAR (transformation(0, 1), -0.628136, 1e-4);
  EXPECT_NEAR (transformation(0, 2), 0.762759, 1e-4);
  EXPECT_NEAR (transformation(0, 3), 15.472, 1e-4);
  EXPECT_NEAR (transformation(1, 0), 0.967397, 1e-4);
  EXPECT_NEAR (transformation(1, 1), -0.252918, 1e-4);
  EXPECT_NEAR (transformation(1, 2), -0.0132578, 1e-4);
  EXPECT_NEAR (transformation(1, 3), -96.6221, 1e-4);
  EXPECT_NEAR (transformation(2, 0), 0.201243, 1e-4);
  EXPECT_NEAR (transformation(2, 1), 0.735852, 1e-4);
  EXPECT_NEAR (transformation(2, 2), 0.646547, 1e-4);
  EXPECT_NEAR (transformation(2, 3), -20.134, 1e-4);
  EXPECT_NEAR (transformation(3, 0), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 1), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 2), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 3), 1.000000, 1e-4);
}
#endif

/* ---[ */
int
main (int argc, char** argv)
{
  if (argc < 4)
  {
    std::cerr << "No test files given. Please download `bun0.pcd`, `bun4.pcd` and `milk_color.pcd` pass their path to the test." << std::endl;
    return (-1);
  }

  // Input
  if (loadPCDFile (argv[1], cloud_source) < 0)
  {
    std::cerr << "Failed to read test file. Please download `bun0.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }
  if (loadPCDFile (argv[2], cloud_target) < 0)
  {
    std::cerr << "Failed to read test file. Please download `bun4.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }
  if (loadPCDFile (argv[3], cloud_with_color) < 0)
  {
    std::cerr << "Failed to read test file. Please download `milk_color.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }

  testing::InitGoogleTest (&argc, argv);
  return (RUN_ALL_TESTS ());

  // Tranpose the cloud_model
  /*for (std::size_t i = 0; i < cloud_model.size (); ++i)
  {
  //  cloud_model[i].z += 1;
  }*/
}
/* ]--- */
