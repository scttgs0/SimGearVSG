// VPBTechnique.hxx -- VirtualPlanetBuilder Effects technique
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2020 Stuart Buchanan

#pragma once

#include <mutex>

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <osgTerrain/TerrainTechnique>
#include <osgTerrain/Locator>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/bvh/BVHMaterial.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/LightBin.hxx>
#include <simgear/scene/tgdb/LineFeatureBin.hxx>
#include <simgear/scene/tgdb/VPBBufferData.hxx>

using namespace osgTerrain;

namespace simgear {

class VPBTechnique : public TerrainTechnique
{
    public:

        VPBTechnique();
        VPBTechnique(const SGReaderWriterOptions* options, const std::string fileName);

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        VPBTechnique(const VPBTechnique&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(osgTerrain, VPBTechnique);

        virtual void init(int dirtyMask, bool assumeMultiThreaded);

        virtual Locator* computeMasterLocator();

        virtual void update(osg::NodeVisitor& nv);

        virtual void cull(osg::NodeVisitor& nv);

        /** Traverse the terrain subgraph.*/
        virtual void traverse(osg::NodeVisitor& nv);

        virtual BVHMaterial* getMaterial(vsg::dvec3 point);
        virtual SGSphered computeBoundingSphere() const;

        virtual void cleanSceneGraph();

        void setFilterBias(float filterBias);
        float getFilterBias() const { return _filterBias; }

        void setFilterWidth(float filterWidth);
        float getFilterWidth() const { return _filterWidth; }

        void setFilterMatrix(const osg::Matrix3& matrix);
        osg::Matrix3& getFilterMatrix() { return _filterMatrix; }
        const osg::Matrix3& getFilterMatrix() const { return _filterMatrix; }

        enum FilterType
        {
            GAUSSIAN,
            SMOOTH,
            SHARPEN
        };

        void setFilterMatrixAs(FilterType filterType);

        void setOptions(const SGReaderWriterOptions* options);

        /** If State is non-zero, this function releases any associated OpenGL objects for
        * the specified graphics context. Otherwise, releases OpenGL objects
        * for all graphics contexts. */
        virtual void releaseGLObjects(vsg::State* = 0) const;

        // Elevation constraints ensure that the terrain mesh is placed underneath objects such as airports.
        // As airports are generated in a separate loading thread, these are static.
        static void addElevationConstraint(vsg::ref_ptr<vsg::Node> constraint);
        static void removeElevationConstraint(vsg::ref_ptr<vsg::Node> constraint);
        static double getConstrainedElevation(vsg::dvec3 ndc, Locator* masterLocator, double vtx_gap);
        static bool checkAgainstElevationConstraints(vsg::dvec3 origin, vsg::dvec3 vertex);

        static void clearConstraints();

        inline static const char* MODEL_OFFSET   = "fg_modelOffset";
        inline static const char* PHOTO_SCENERY  = "fg_photoScenery";

    protected:

        virtual ~VPBTechnique();

        class VertexNormalGenerator
        {
        public:

            typedef std::vector<int> Indices;
            typedef std::pair< vsg::ref_ptr<osg::Vec2Array>, Locator* > TexCoordLocatorPair;
            typedef std::map< Layer*, TexCoordLocatorPair > LayerToTexCoordMap;

            VertexNormalGenerator(Locator* masterLocator, const vsg::dvec3& centerModel, int numRows, int numColmns, float scaleHeight, float vtx_gap, bool createSkirt, bool useTessellation);

            void populateCenter(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas, osgTerrain::TerrainTile* tile, osg::Vec2Array* texcoords1, osg::Vec2Array* texcoords2);
            void populateSeaLevel();
            void populateLeftBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas);
            void populateRightBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas);
            void populateAboveBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas);
            void populateBelowBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas);

            enum class Corner {
                BOTTOM_LEFT,
                BOTTOM_RIGHT,
                TOP_LEFT,
                TOP_RIGHT
            };
            void populateCorner(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, vsg::ref_ptr<Atlas> atlas, Corner corner);

            void computeNormals();

            //  Convert NDC coordinates into the model coordinates, which centered on the model center and are Z-up
            vsg::dvec3 convertLocalToModel(vsg::dvec3 ndc)
            {
                vsg::dvec3 model;
                _masterLocator->convertLocalToModel(ndc, model);
                return _ZUpRotationMatrix * (model - _centerModel);
                return (model - _centerModel);
            }

            unsigned int capacity() const { return _vertices->capacity(); }

            // Tessellation case - no normal required
            inline void setVertex(int c, int r, const vsg::vec3& v)
            {
                int& i = index(c,r);
                if (i==0) {
                    i = _vertices->size() + 1;
                    _vertices->push_back(v);
                } else {
                    if (r<0 || r>=_numRows || c<0 || c>=_numColumns) {
                        (*_vertices)[i-1] = v;
                    } else {
                        // average the vertex positions
                        (*_vertices)[i-1] = ((*_vertices)[i-1] + v)*0.5f;
                    }
                }
            }

            // Non-tessellation case - normal and boundaries required
            inline void setVertex(int c, int r, const vsg::vec3& v, const vsg::vec3& n)
            {
                int& i = index(c,r);
                if (i==0) {
                    if (r<0 || r>=_numRows || c<0 || c>=_numColumns) {
                        i = -(1+static_cast<int>(_boundaryVertices->size()));
                        _boundaryVertices->push_back(v);
                    } else {
                        i = _vertices->size() + 1;
                        _vertices->push_back(v);
                        _normals->push_back(n);
                    }
                } else if (i<0) {
                    (*_boundaryVertices)[-i-1] = v;
                } else {
                    // average the vertex positions
                    (*_vertices)[i-1] = ((*_vertices)[i-1] + v)*0.5f;
                    (*_normals)[i-1] = n;
                }
            }

            inline bool computeNormal(int c, int r, vsg::vec3& n) const
            {
#if 1
                return computeNormalWithNoDiagonals(c,r,n);
#else
                return computeNormalWithDiagonals(c,r,n);
#endif
            }

            inline bool computeNormalWithNoDiagonals(int c, int r, vsg::vec3& n) const
            {
                vsg::vec3 center;
                bool center_valid  = vertex(c, r,  center);
                if (!center_valid) return false;

                vsg::vec3 left, right, top,  bottom;
                bool left_valid  = vertex(c-1, r,  left);
                bool right_valid = vertex(c+1, r,   right);
                bool bottom_valid = vertex(c,   r-1, bottom);
                bool top_valid = vertex(c,   r+1, top);

                vsg::vec3 dx(0.0f,0.0f,0.0f);
                vsg::vec3 dy(0.0f,0.0f,0.0f);
                vsg::vec3 zero(0.0f,0.0f,0.0f);
                if (left_valid)
                {
                    dx += center-left;
                }
                if (right_valid)
                {
                    dx += right-center;
                }
                if (bottom_valid)
                {
                    dy += center-bottom;
                }
                if (top_valid)
                {
                    dy += top-center;
                }

                if (dx==zero || dy==zero) return false;

                n = dx ^ dy;
                return n.normalize() != 0.0f;
            }

            inline bool computeNormalWithDiagonals(int c, int r, vsg::vec3& n) const
            {
                vsg::vec3 center;
                bool center_valid  = vertex(c, r,  center);
                if (!center_valid) return false;

                vsg::vec3 top_left, top_right, bottom_left, bottom_right;
                bool top_left_valid  = vertex(c-1, r+1,  top_left);
                bool top_right_valid  = vertex(c+1, r+1,  top_right);
                bool bottom_left_valid  = vertex(c-1, r-1,  bottom_left);
                bool bottom_right_valid  = vertex(c+1, r-1,  bottom_right);

                vsg::vec3 left, right, top,  bottom;
                bool left_valid  = vertex(c-1, r,  left);
                bool right_valid = vertex(c+1, r,   right);
                bool bottom_valid = vertex(c,   r-1, bottom);
                bool top_valid = vertex(c,   r+1, top);

                vsg::vec3 dx(0.0f,0.0f,0.0f);
                vsg::vec3 dy(0.0f,0.0f,0.0f);
                vsg::vec3 zero(0.0f,0.0f,0.0f);
                const float ratio = 0.5f;
                if (left_valid)
                {
                    dx = center-left;
                    if (top_left_valid) dy += (top_left-left)*ratio;
                    if (bottom_left_valid) dy += (left-bottom_left)*ratio;
                }
                if (right_valid)
                {
                    dx = right-center;
                    if (top_right_valid) dy += (top_right-right)*ratio;
                    if (bottom_right_valid) dy += (right-bottom_right)*ratio;
                }
                if (bottom_valid)
                {
                    dy += center-bottom;
                    if (bottom_left_valid) dx += (bottom-bottom_left)*ratio;
                    if (bottom_right_valid) dx += (bottom_right-bottom)*ratio;
                }
                if (top_valid)
                {
                    dy += top-center;
                    if (top_left_valid) dx += (top-top_left)*ratio;
                    if (top_right_valid) dx += (top_right-top)*ratio;
                }

                if (dx==zero || dy==zero) return false;

                n = dx ^ dy;
                return n.normalize() != 0.0f;
            }

            inline int& index(int c, int r) { return _indices[(r+1)*(_numColumns+2)+c+1]; }

            inline int index(int c, int r) const { return _indices[(r+1)*(_numColumns+2)+c+1]; }

            inline int vertex_index(int c, int r) const { int i = _indices[(r+1)*(_numColumns+2)+c+1]; return i-1; }

            inline bool vertex(int c, int r, vsg::vec3& v) const
            {
                int i = index(c,r);
                if (i==0) return false;
                if (i<0) v = (*_boundaryVertices)[-i-1];
                else v = (*_vertices)[i-1];
                return true;
            }

            bool hasSea() { return _hasSea; }

            Locator*                        _masterLocator;
            const vsg::dvec3                _centerModel;
            int                             _numRows;
            int                             _numColumns;
            float                           _scaleHeight;
            float                           _constraint_vtx_gap;

            Indices                         _indices;

            vsg::ref_ptr<vsg::vec3Array>    _vertices;
            vsg::ref_ptr<vsg::vec3Array>    _normals;

            vsg::ref_ptr<vsg::vec3Array>    _sea_vertices;
            vsg::ref_ptr<vsg::vec3Array>    _sea_normals;

            std::vector<float>              _elevationConstraints;

            vsg::ref_ptr<vsg::vec3Array>    _boundaryVertices;
            bool                            _useTessellation;
            bool                            _hasSea;

            vsg::mat4                     _ZUpRotationMatrix;
        };


        virtual vsg::dvec3 computeCenter(BufferData& buffer);
        virtual vsg::dvec3 computeCenterModel(BufferData& buffer);
        const virtual SGGeod computeCenterGeod(BufferData& buffer);

        virtual void generateGeometry(BufferData& buffer, const vsg::dvec3& centerModel, vsg::ref_ptr<SGMaterialCache> matcache);

        virtual void applyColorLayers(BufferData& buffer, vsg::ref_ptr<SGMaterialCache> matcache);

        virtual double det2(const vsg::dvec2 a, const vsg::dvec2 b);
        virtual int getLandclass(const vsg::dvec2 p);

        virtual void applyMaterials(BufferData& buffer, vsg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc);
        virtual void applyMaterialsTesselated(BufferData& buffer, vsg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc);
        virtual void applyMaterialsTriangles(BufferData& buffer, vsg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc);

        virtual vsg::dvec4 catmull_rom_interp_basis(const float t);

        virtual vsg::Image* generateWaterTexture(Atlas* atlas);
        virtual osg::Texture2D* getCoastlineTexture(const SGBucket bucket);

        static void updateStats(int tileLevel, float loadTime);

        // Check a given vertex against any constraints  E.g. to ensure we
        // don't get objects like trees sprouting from roads or runways.
        bool checkAgainstRandomObjectsConstraints(BufferData& buffer,
                                                  vsg::dvec3 origin, vsg::dvec3 vertex);


        bool checkAgainstWaterConstraints(BufferData& buffer, vsg::dvec2 point);

        OpenThreads::Mutex                  _writeBufferMutex;
        vsg::ref_ptr<BufferData>            _currentBufferData;
        vsg::ref_ptr<BufferData>            _newBufferData;

        float                               _filterBias;
        vsg::ref_ptr<osg::Uniform>          _filterBiasUniform;
        float                               _filterWidth;
        vsg::ref_ptr<osg::Uniform>          _filterWidthUniform;
        osg::Matrix3                        _filterMatrix;
        vsg::ref_ptr<osg::Uniform>          _filterMatrixUniform;
        vsg::ref_ptr<SGReaderWriterOptions> _options;
        const std::string                   _fileName;
        vsg::ref_ptr<vsg::Group>            _randomObjectsConstraintGroup;
        bool                                _useTessellation;
        vsg::ref_ptr<osg::Referenced>       _databaseRequest;

        inline static vsg::ref_ptr<vsg::Group>  _elevationConstraintGroup = new vsg::Group();
        inline static std::shared_mutex _elevationConstraintMutex;  // protects the _elevationConstraintGroup;

        inline static std::mutex _stats_mutex; // Protects the loading statistics and other static properties
        typedef std::pair<unsigned int, float> LoadStat;
        inline static std::map<int, LoadStat> _loadStats;
        inline static SGPropertyNode* _statsPropertyNode;
        inline static SGPropertyNode* _useTessellationPropNode;
};

}; // namespace simgear
