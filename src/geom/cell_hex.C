// The libMesh Finite Element Library.
// Copyright (C) 2002-2017 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


// C++ includes
#include <algorithm> // for std::min, std::max

// Local includes
#include "libmesh/cell_hex.h"
#include "libmesh/cell_hex8.h"
#include "libmesh/face_quad4.h"



namespace libMesh
{



// ------------------------------------------------------------
// Hex class static member initializations


// We need to require C++11...
const Real Hex::_master_points[27][3] =
  {
    {-1, -1, -1},
    {1, -1, -1},
    {1, 1, -1},
    {-1, 1, -1},
    {-1, -1, 1},
    {1, -1, 1},
    {1, 1, 1},
    {-1, 1, 1},
    {0, -1, -1},
    {1, 0, -1},
    {0, 1, -1},
    {-1, 0, -1},
    {-1, -1, 0},
    {1, -1, 0},
    {1, 1, 0},
    {-1, 1, 0},
    {0, -1, 1},
    {1, 0, 1},
    {0, 1, 1},
    {-1, 0, 1},
    {0, 0, -1},
    {0, -1, 0},
    {1, 0, 0},
    {0, 1, 0},
    {-1, 0, 0},
    {0, 0, 1}
  };




// ------------------------------------------------------------
// Hex class member functions
dof_id_type Hex::key (const unsigned int s) const
{
  libmesh_assert_less (s, this->n_sides());

  return this->compute_key(this->node_id(Hex8::side_nodes_map[s][0]),
                           this->node_id(Hex8::side_nodes_map[s][1]),
                           this->node_id(Hex8::side_nodes_map[s][2]),
                           this->node_id(Hex8::side_nodes_map[s][3]));
}



unsigned int Hex::which_node_am_i(unsigned int side,
                                  unsigned int side_node) const
{
  libmesh_assert_less (side, this->n_sides());
  libmesh_assert_less (side_node, 4);

  return Hex8::side_nodes_map[side][side_node];
}



std::unique_ptr<Elem> Hex::side_ptr (const unsigned int i)
{
  libmesh_assert_less (i, this->n_sides());

  Elem * face = new Quad4;

  for (unsigned n=0; n<face->n_nodes(); ++n)
    face->set_node(n) = this->node_ptr(Hex8::side_nodes_map[i][n]);

  return std::unique_ptr<Elem>(face);
}



bool Hex::is_child_on_side(const unsigned int c,
                           const unsigned int s) const
{
  libmesh_assert_less (c, this->n_children());
  libmesh_assert_less (s, this->n_sides());

  // This array maps the Hex8 node numbering to the Hex8 child
  // numbering.  I.e.
  //   node 6 touches child 7, and
  //   node 7 touches child 6, etc.
  const unsigned int node_child_map[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };

  for (unsigned int i = 0; i != 4; ++i)
    if (node_child_map[Hex8::side_nodes_map[s][i]] == c)
      return true;

  return false;
}



bool Hex::is_edge_on_side(const unsigned int e,
                          const unsigned int s) const
{
  libmesh_assert_less (e, this->n_edges());
  libmesh_assert_less (s, this->n_sides());

  return (is_node_on_side(Hex8::edge_nodes_map[e][0],s) &&
          is_node_on_side(Hex8::edge_nodes_map[e][1],s));
}



unsigned int Hex::opposite_side(const unsigned int side_in) const
{
  libmesh_assert_less (side_in, 6);
  static const unsigned char hex_opposites[6] = {5, 3, 4, 1, 2, 0};
  return hex_opposites[side_in];
}



unsigned int Hex::opposite_node(const unsigned int node_in,
                                const unsigned int side_in) const
{
  libmesh_assert_less (node_in, 26);
  libmesh_assert_less (node_in, this->n_nodes());
  libmesh_assert_less (side_in, this->n_sides());
  libmesh_assert(this->is_node_on_side(node_in, side_in));

  static const unsigned char side05_nodes_map[] =
    {4, 5, 6, 7, 0, 1, 2, 3, 16, 17, 18, 19, 255, 255, 255, 255, 8, 9, 10, 11, 25, 255, 255, 255, 255, 20};
  static const unsigned char side13_nodes_map[] =
    {3, 2, 1, 0, 7, 6, 5, 4, 10, 255, 8, 255, 15, 14, 13, 12, 18, 255, 16, 255, 255, 23, 255, 21, 255, 255};
  static const unsigned char side24_nodes_map[] =
    {1, 0, 3, 2, 5, 4, 7, 6, 255, 11, 255, 9, 13, 12, 15, 14, 255, 19, 255, 17, 255, 255, 24, 255, 22, 255};

  switch (side_in)
    {
    case 0:
    case 5:
      return side05_nodes_map[node_in];
    case 1:
    case 3:
      return side13_nodes_map[node_in];
    case 2:
    case 4:
      return side24_nodes_map[node_in];
    default:
      libmesh_error_msg("Unsupported side_in = " << side_in);
    }

  libmesh_error_msg("We'll never get here!");
  return 255;
}



Real Hex::quality (const ElemQuality q) const
{
  switch (q)
    {

      /**
       * Compute the min/max diagonal ratio.
       * Source: CUBIT User's Manual.
       */
    case DIAGONAL:
      {
        // Diagonal between node 0 and node 6
        const Real d06 = this->length(0,6);

        // Diagonal between node 3 and node 5
        const Real d35 = this->length(3,5);

        // Diagonal between node 1 and node 7
        const Real d17 = this->length(1,7);

        // Diagonal between node 2 and node 4
        const Real d24 = this->length(2,4);

        // Find the biggest and smallest diagonals
        const Real min = std::min(d06, std::min(d35, std::min(d17, d24)));
        const Real max = std::max(d06, std::max(d35, std::max(d17, d24)));

        libmesh_assert_not_equal_to (max, 0.0);

        return min / max;

        break;
      }

      /**
       * Minimum ratio of lengths derived from opposite edges.
       * Source: CUBIT User's Manual.
       */
    case TAPER:
      {

        /**
         * Compute the side lengths.
         */
        const Real d01 = this->length(0,1);
        const Real d12 = this->length(1,2);
        const Real d23 = this->length(2,3);
        const Real d03 = this->length(0,3);
        const Real d45 = this->length(4,5);
        const Real d56 = this->length(5,6);
        const Real d67 = this->length(6,7);
        const Real d47 = this->length(4,7);
        const Real d04 = this->length(0,4);
        const Real d15 = this->length(1,5);
        const Real d37 = this->length(3,7);
        const Real d26 = this->length(2,6);

        std::vector<Real> edge_ratios(12);
        // Front
        edge_ratios[0] = std::min(d01, d45) / std::max(d01, d45);
        edge_ratios[1] = std::min(d04, d15) / std::max(d04, d15);

        // Right
        edge_ratios[2] = std::min(d15, d26) / std::max(d15, d26);
        edge_ratios[3] = std::min(d12, d56) / std::max(d12, d56);

        // Back
        edge_ratios[4] = std::min(d67, d23) / std::max(d67, d23);
        edge_ratios[5] = std::min(d26, d37) / std::max(d26, d37);

        // Left
        edge_ratios[6] = std::min(d04, d37) / std::max(d04, d37);
        edge_ratios[7] = std::min(d03, d47) / std::max(d03, d47);

        // Bottom
        edge_ratios[8] = std::min(d01, d23) / std::max(d01, d23);
        edge_ratios[9] = std::min(d03, d12) / std::max(d03, d12);

        // Top
        edge_ratios[10] = std::min(d45, d67) / std::max(d45, d67);
        edge_ratios[11] = std::min(d56, d47) / std::max(d56, d47);

        return *(std::min_element(edge_ratios.begin(), edge_ratios.end())) ;

        break;
      }


      /**
       * Minimum edge length divided by max diagonal length.
       * Source: CUBIT User's Manual.
       */
    case STRETCH:
      {
        const Real sqrt3 = 1.73205080756888;

        /**
         * Compute the maximum diagonal.
         */
        const Real d06 = this->length(0,6);
        const Real d17 = this->length(1,7);
        const Real d35 = this->length(3,5);
        const Real d24 = this->length(2,4);
        const Real max_diag = std::max(d06, std::max(d17, std::max(d35, d24)));

        libmesh_assert_not_equal_to ( max_diag, 0.0 );

        /**
         * Compute the minimum edge length.
         */
        std::vector<Real> edges(12);
        edges[0]  = this->length(0,1);
        edges[1]  = this->length(1,2);
        edges[2]  = this->length(2,3);
        edges[3]  = this->length(0,3);
        edges[4]  = this->length(4,5);
        edges[5]  = this->length(5,6);
        edges[6]  = this->length(6,7);
        edges[7]  = this->length(4,7);
        edges[8]  = this->length(0,4);
        edges[9]  = this->length(1,5);
        edges[10] = this->length(2,6);
        edges[11] = this->length(3,7);

        const Real min_edge = *(std::min_element(edges.begin(), edges.end()));
        return sqrt3 * min_edge / max_diag ;
      }


      /**
       * I don't know what to do for this metric.
       * Maybe the base class knows...
       */
    default:
      return Elem::quality(q);
    }

  libmesh_error_msg("We'll never get here!");
  return 0.;
}



std::pair<Real, Real> Hex::qual_bounds (const ElemQuality q) const
{
  std::pair<Real, Real> bounds;

  switch (q)
    {

    case ASPECT_RATIO:
      bounds.first  = 1.;
      bounds.second = 4.;
      break;

    case SKEW:
      bounds.first  = 0.;
      bounds.second = 0.5;
      break;

    case SHEAR:
    case SHAPE:
      bounds.first  = 0.3;
      bounds.second = 1.;
      break;

    case CONDITION:
      bounds.first  = 1.;
      bounds.second = 8.;
      break;

    case JACOBIAN:
      bounds.first  = 0.5;
      bounds.second = 1.;
      break;

    case DISTORTION:
      bounds.first  = 0.6;
      bounds.second = 1.;
      break;

    case TAPER:
      bounds.first  = 0.;
      bounds.second = 0.4;
      break;

    case STRETCH:
      bounds.first  = 0.25;
      bounds.second = 1.;
      break;

    case DIAGONAL:
      bounds.first  = 0.65;
      bounds.second = 1.;
      break;

    case SIZE:
      bounds.first  = 0.5;
      bounds.second = 1.;
      break;

    default:
      libMesh::out << "Warning: Invalid quality measure chosen." << std::endl;
      bounds.first  = -1;
      bounds.second = -1;
    }

  return bounds;
}



const unsigned short int Hex::_second_order_vertex_child_number[27] =
  {
    99,99,99,99,99,99,99,99, // Vertices
    0,1,2,0,0,1,2,3,4,5,6,5, // Edges
    0,0,1,2,0,4,             // Faces
    0                        // Interior
  };



const unsigned short int Hex::_second_order_vertex_child_index[27] =
  {
    99,99,99,99,99,99,99,99, // Vertices
    1,2,3,3,4,5,6,7,5,6,7,7, // Edges
    2,5,6,7,7,6,             // Faces
    6                        // Interior
  };


const unsigned short int Hex::_second_order_adjacent_vertices[12][2] =
  {
    { 0,  1}, // vertices adjacent to node 8
    { 1,  2}, // vertices adjacent to node 9
    { 2,  3}, // vertices adjacent to node 10
    { 0,  3}, // vertices adjacent to node 11

    { 0,  4}, // vertices adjacent to node 12
    { 1,  5}, // vertices adjacent to node 13
    { 2,  6}, // vertices adjacent to node 14
    { 3,  7}, // vertices adjacent to node 15

    { 4,  5}, // vertices adjacent to node 16
    { 5,  6}, // vertices adjacent to node 17
    { 6,  7}, // vertices adjacent to node 18
    { 4,  7}  // vertices adjacent to node 19
  };


#ifdef LIBMESH_ENABLE_AMR

// We number 125 "possible node locations" for a 2x2x2 refinement of
// hexes with up to 3x3x3 nodes each
const int Hex::_child_node_lookup[8][27] =
  {
    // node lookup for child 0 (near node 0)
    { 0, 2, 12, 10,  50, 52, 62, 60,  1, 7, 11, 5,  25, 27, 37, 35,
      51, 57, 61, 55,  6,  26, 32, 36, 30,  56,  31},

    // node lookup for child 1 (near node 1)
    { 2, 4, 14, 12,  52, 54, 64, 62,  3, 9, 13, 7,  27, 29, 39, 37,
      53, 59, 63, 57,  8,  28, 34, 38, 32,  58,  33},

    // node lookup for child 2 (near node 3)
    { 10, 12, 22, 20,  60, 62, 72, 70,  11, 17, 21, 15,  35, 37, 47, 45,
      61, 67, 71, 65,  16,  36, 42, 46, 40,  66,  41},

    // node lookup for child 3 (near node 2)
    { 12, 14, 24, 22,  62, 64, 74, 72,  13, 19, 23, 17,  37, 39, 49, 47,
      63, 69, 73, 67,  18,  38, 44, 48, 42,  68,  43},

    // node lookup for child 4 (near node 4)
    { 50, 52, 62, 60,  100, 102, 112, 110,  51, 57, 61, 55,  75, 77, 87, 85,
      101, 107, 111, 105,  56,  76, 82, 86, 80,  106,  81},

    // node lookup for child 5 (near node 5)
    { 52, 54, 64, 62,  102, 104, 114, 112,  53, 59, 63, 57,  77, 79, 89, 87,
      103, 109, 113, 107,  58,  78, 84, 88, 82,  108,  93},

    // node lookup for child 6 (near node 7)
    { 60, 62, 72, 70,  110, 112, 122, 120,  61, 67, 71, 65,  85, 87, 97, 95,
      111, 117, 121, 115,  66,  86, 92, 96, 90,  116,  91},

    // node lookup for child 7 (near node 6)
    { 62, 64, 74, 72,  112, 114, 124, 122,  63, 69, 73, 67,  87, 89, 99, 97,
      113, 119, 123, 117,  68,  88, 94, 98, 92,  118,  103}
  };

#endif // LIBMESH_ENABLE_AMR


} // namespace libMesh
