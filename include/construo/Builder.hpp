///\todo fix GPL


#include <construo/ImageFileLoader.h>
#include <construo/ImagePatch.h>

///\todo remove
#include <construo/Visualizer.h>

BEGIN_CRUSTA

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
Builder(const std::string& spheroidName, const uint size[2])
{
///\todo Frak this is retarded. Reason so far is the getRefinement from scope
assert(size[0]==size[1]);

    tileSize[0] = size[0];
    tileSize[1] = size[1];

    globe = new Globe(spheroidName, tileSize);

///\todo for now hardcode the subsampling filter
    subsamplingFilter = Filter::createPointFilter();

    scopeBuf          = new Scope::Scalar[tileSize[0]*tileSize[1]*3];
    nodeDataBuf       = new PixelParam[tileSize[0]*tileSize[1]];
    nodeDataSampleBuf = new PixelParam[tileSize[0]*tileSize[1]];
    //the -3 takes into account the shared edges of the tiles
    domainSize[0] = 4*tileSize[0] - 3;
    domainSize[1] = 4*tileSize[1] - 3;
    domainBuf     = new PixelParam[domainSize[0]*domainSize[1]];
}

template <typename PixelParam, typename PolyhedronParam>
Builder<PixelParam, PolyhedronParam>::
~Builder()
{
    delete globe;
    for (typename PatchPtrs::iterator it=patches.begin(); it!=patches.end();
         ++it)
    {
        delete *it;
    }
    delete[] scopeBuf;
    delete[] nodeDataBuf;
    delete[] nodeDataSampleBuf;
    delete[] domainBuf;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
refine(Node* node)
{
    //try to load the missing children from the quadtree file
    if (node->children == NULL)
        node->loadMissingChildren();

    //if loading failed then we just have to create new children
    if (node->children == NULL)
    {
        node->createChildren();
        typename Node::State::File::TileIndex childIndices[4];
        for (uint i=0; i<4; ++i)
        {
            Node* child      = &node->children[i];
            child->tileIndex = child->treeState->file->appendTile();
            assert(child->tileIndex!=Node::State::File::INVALID_TILEINDEX);
///\todo should I dump default-value-initialized to file here?
            childIndices[i]  = child->tileIndex;
        }
        //link the parent with the new children in the file
        node->treeState->file->writeTile(node->tileIndex, childIndices);
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
flagAncestorsForUpdate(Node* node)
{
    while (node != NULL)
    {
        node->mustBeUpdated = true;
        node = node->parent;
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
sourceFinest(Node* node, Patch* imgPatch, uint overlap)
{
    //convert the node's bounding box to the image space
    Box nodeImgBox = imgPatch->transform->worldToImage(
        node->coverage.getBoundingBox());
    //use the box to load an appropriate rectangle from the image file
    uint rectOrigin[2];
    uint rectSize[2];
    for (uint i=0; i<2; i++)
    {
///\todo abstract the sampler
        //assume bilinear interpolation here
        rectOrigin[i] = static_cast<uint>(Math::floor(nodeImgBox.min[i]));
        rectSize[i]   = static_cast<uint>(Math::ceil(nodeImgBox.max[i])) + 1 -
                                                     rectOrigin[i];
    }
    PixelParam* rectBuffer = new PixelParam[rectSize[0] * rectSize[1]];
    imgPatch->image->readRectangle(rectOrigin, rectSize, rectBuffer);

    //determine the sample positions for the scope
///\todo enhance get Refinement such that is can produce non-uniform samples
    node->scope.getRefinement(tileSize[0], scopeBuf);
///\todo remove
#if 0
{
    Visualizer::Floats scopeRef;
    uint numScopeSamples = tileSize[0]*tileSize[1]*3;
    scopeRef.resize(numScopeSamples);
    float* out = &scopeRef[0];
    for (double* in=&scopeBuf[0]; in<&scopeBuf[0]+numScopeSamples; in+=3,out+=3)
    {
        Scope::Scalar len = sqrt(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
        Point p(atan2(in[1], in[0]), acos(in[2]/len));
        out[0] = p[0];
        out[1] =  0.0;
        out[2] = p[1];
    }
    Visualizer::addPrimitive(GL_POINTS, scopeRef);
    Visualizer::show();
}
#endif
    //prepare the node's data buffer
    node->data = nodeDataBuf;
    node->treeState->file->readTile(node->tileIndex, node->data);
    
    //resample pixel values for the node from the read rectangle
    Scope::Scalar* endPtr = scopeBuf + tileSize[0]*tileSize[1];
    PixelParam* dataPtr   = node->data;
    for (Scope::Scalar* curPtr=scopeBuf; curPtr!=endPtr; curPtr+=3, ++dataPtr)
    {
        /* transform the sample position into spherical space and then to the
           image patch's image coordinates: */
        Scope::Scalar len =
            sqrt(curPtr[0]*curPtr[0]+curPtr[1]*curPtr[1]+curPtr[2]*curPtr[2]);
        Point p(atan2(curPtr[1], curPtr[0]), acos(curPtr[2]/len));
        p = imgPatch->transform->worldToImage(p);
        
        /* Check if the point is inside the image patch's coverage region: */
        if(overlap==SphereCoverage::ISCONTAINED ||
           imgPatch->imageCoverage->contains(p))
        {
            /* Sample the point: */
            int ip[2];
            double d[2];
            for(uint i=0; i<2; i++)
            {
                double pFloor = Math::floor(p[i]);
                d[i]          = p[i] - pFloor;
                ip[i]         = static_cast<int>(pFloor) - rectOrigin[i];
            }
            PixelParam* basePtr = rectBuffer + (ip[1]*rectSize[0] + ip[0]);
            PixelParam one = (1.0-d[0])*basePtr[0] + d[0]*(basePtr[1]);
            PixelParam two = (1.0-d[0])*basePtr[rectSize[0]] +
                             d[0]*basePtr[rectSize[0]+1];
            *dataPtr = (1.0-d[1])*one + d[1]*two;
        }
    }

    //clean up the temporary image uffer
    delete[] rectBuffer;

    //commit the data to file
    QuadtreeTileHeader<PixelParam> header;
    header.reset(node);
    node->treeState->file->writeTile(node->tileIndex, header, node->data);

    node->data = NULL;
    
    static const int offsets[4][3][2] = {
        { {-1, 0}, {-1,-1}, { 0,-1} }, { { 0,-1}, { 1,-1}, { 1, 0} },
        { {-1, 0}, {-1, 1}, { 0, 1} }, { { 0, 1}, { 1, 1}, { 1, 0} }
    };
    if (node->parent != NULL)
    {
        //make sure that the parent dependent on this new data is also updated
        flagAncestorsForUpdate(node->parent);
        
        //we musn't forget to update all the adjacent non-sibling nodes either
        int off[2];
        Node* kin;
        for (uint i=0; i<3; ++i)
        {
            off[0] = offsets[node->treeIndex.child][i][0];
            off[1] = offsets[node->treeIndex.child][i][1];
            node->parent->getKin(kin, off);
            if (kin != NULL)
                flagAncestorsForUpdate(kin);
        }
    }
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam, typename PolyhedronParam>
int Builder<PixelParam, PolyhedronParam>::
updateFiner(Node* node, Patch* imgPatch, Point::Scalar imgResolution)
{
///\todo remove
//Visualizer::addPrimitive(GL_LINES, node->coverage);
//Visualizer::peek();

    //check for an overlap
    uint overlap = node->coverage.overlaps(*(imgPatch->sphereCoverage));
    if (overlap == SphereCoverage::SEPARATE)
        return 0;

    //recurse to children if the resolution of the node is too coarse
    if (node->resolution > imgResolution)
    {
        int depth = 0;
        refine(node);
        for (uint i=0; i<4; ++i)
        {
            depth = std::max(updateFiner(&node->children[i], imgPatch,
                                         imgResolution), depth);
        }
        return depth;
    }

    //this node has the appropriate resolution
//Visualizer::show();
    sourceFinest(node, imgPatch, overlap);
    return node->treeIndex.level;
}
    
template <typename PixelParam, typename PolyhedronParam>
int Builder<PixelParam, PolyhedronParam>::
updateFinestLevels()
{
    int depth = 0;
    //iterate over all the image patches to source
    for (typename PatchPtrs::iterator pIt=patches.begin(); pIt!=patches.end();
         ++pIt)
    {
///\todo remove
//Visualizer::clear();
//Visualizer::addPrimitive(GL_LINES, *((*pIt)->sphereCoverage));

        //grab the smallest resolution from the image
        Point::Scalar imgResolution;
        {
            const Point::Scalar* resolutions = (*pIt)->transform->getScale();
/**\todo the scales are used in the transform to flip/flop the image so they may
         be negative. Need to have a proper image sample resolution and not
         misuse the transform scale */
            imgResolution = std::min(fabs(resolutions[0]),fabs(resolutions[1]));
        }

        //iterate over all the spheroid's base patches to determine overlap
        for (typename Globe::BaseNodes::iterator bIt=globe->baseNodes.begin();
             bIt!=globe->baseNodes.end(); ++bIt)
        {
            depth = std::max(updateFiner(&(*bIt), *pIt, imgResolution), depth);
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(&(*bIt));
//Visualizer::show();
        }
    }

    return depth;
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
prepareSubsamplingDomain(Node* node)
{
    Node* kin     = NULL;

    PixelParam* domain = domainBuf + (tileSize[1]-1)*domainSize[0] +
                         tileSize[0]-1;

    int domainOff[2];
    int nodeOff[2];
    for (domainOff[1]=-1; domainOff[1]<=2; ++domainOff[1])
    {
        for (domainOff[0]=-1; domainOff[0]<=2; ++domainOff[0])
        {
        //- retrieve the kin
            nodeOff[0] = domainOff[0];
            nodeOff[1] = domainOff[1];
            node->getKin(kin, nodeOff, true, 1);

        //- retrieve data as appropriate
            //default to blank data
            const PixelParam* data = node->treeState->file->getBlank();
            
            //read the data from file
            if (kin != NULL)
            {
                assert(kin->tileIndex!=Node::State::File::INVALID_TILEINDEX);
                if (nodeOff[0]==0 && nodeOff[1]==0)
                {
                    //we're grabbing data from a same leveled kin
                    kin->treeState->file->readTile(kin->tileIndex, nodeDataBuf);
                    data = nodeDataBuf;
                }
                else
                {
                    //need to sample coarser level node as the kin replacement
                    kin->treeState->file->readTile(kin->tileIndex,
                                                   nodeDataSampleBuf);
                    //determine the resample step size
                    double scale = 1;
                    for (uint i=node->treeIndex.level-1;
                         i<kin->treeIndex.level; ++i)
                    {
                        scale *= 0.5;
                    }
                    double step[2] = {1.0/(tileSize[0]-1), 1.0/(tileSize[1]-1)};
                    step[0] *= scale;
                    step[1] *= scale;
///\todo abstract out the filtering. Currently using bilinear filtering
                    double d[2];
                    uint ny=0;
                    PixelParam* wbase = nodeDataBuf;
                    for (double y=nodeOff[1]*scale; ny<tileSize[1];
                         ++ny, y+=step[1])
                    {
                        double py = y * (tileSize[1]-1);
                        double fy = floor(py);
                        d[1]      = py - fy;
                        uint iy   = static_cast<uint>(fy);
                        if (iy == tileSize[1])
                        {
                            --iy;
                            d[1] = 1.0;
                        }
                        uint oy = iy * tileSize[0];

                        uint nx=0;
                        for (double x=nodeOff[0]*scale; nx<tileSize[0];
                             ++nx, x+=step[0], ++wbase)
                        {
                            double px = x * (tileSize[0]-1);
                            double fx = floor(px);
                            d[0]      = px - fx;
                            uint ix   = static_cast<uint>(fx);
                            if (ix == tileSize[0])
                            {
                                --ix;
                                d[0] = 1.0;
                            }

                            PixelParam* rbase = nodeDataSampleBuf + oy + ix;

                            PixelParam one = (1.0-d[0]) * rbase[0] +
                                                   d[0] * rbase[1];
                            PixelParam two = (1.0-d[0]) * rbase[tileSize[0]] +
                                                   d[0] * rbase[tileSize[0]+1];

                            *wbase = (1.0-d[1])*one + d[1]*two;
                        }
                    }
                    data = nodeDataBuf;
                }
            }
            
        //- insert the data at the appropriate location
            PixelParam* at = domain +
                             domainOff[1] * (tileSize[1]-1) * domainSize[0] +
                             domainOff[0] * (tileSize[0]-1);
            for (uint y=0; y<tileSize[1]; ++y)
            {
                memcpy(at + y*domainSize[0], data+y*tileSize[0],
                       tileSize[0]*sizeof(PixelParam));
            }
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarser(Node* node, int level)
{
//Visualizer::addPrimitive(GL_LINES, node->coverage);
//Visualizer::peek();
    //the nodes on the way must be flagged for update
    if (!node->mustBeUpdated)
        return;

//- recurse until we've hit the requested level
    if (node->treeIndex.level!=static_cast<uint>(level) && node->children!=NULL)
    {
        for (uint i=0; i<4; ++i)
            updateCoarser(&(node->children[i]), level);
        return;
    }

//- we've reached a node that must be updated
    prepareSubsamplingDomain(node);

    /* walk the pixels of the node's data and performed filtered look-ups into
       the domain */
    PixelParam* data = nodeDataBuf;
    PixelParam* domain;
    for (domain = domainBuf +   (tileSize[1]-1)*domainSize[0]+  (tileSize[0]-1);
         domain<= domainBuf + 3*(tileSize[1]-1)*domainSize[0]+3*(tileSize[0]-1);
         domain+= 2*domainSize[0] - (2*(tileSize[0]-1) + 1))
    {
        for (uint i=0; i<tileSize[0]; ++i, domain+=2, ++data)
            *data = subsamplingFilter.lookup(domain, domainSize[0]);
    }

    //commit the data to file
    node->data = nodeDataBuf;

    QuadtreeTileHeader<PixelParam> header;
    header.reset(node);
    node->treeState->file->writeTile(node->tileIndex, header, node->data);
    
    node->data = NULL;
///\todo this is debugging code to check tree consistency
//verifyQuadtreeFile(node);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
updateCoarserLevels(int depth)
{
    if (depth==0)
        return;

    for (int level=depth-1; level>=0; --level)
    {
        for (typename Globe::BaseNodes::iterator it=globe->baseNodes.begin();
             it!=globe->baseNodes.end(); ++it)
        {
//Visualizer::clear();
//Visualizer::addPrimitive(GL_LINES, it->coverage);
//Visualizer::show();
            //traverse the tree and update the next level
            updateCoarser(&(*it), level);
//verifyQuadtreeFile(&(*it));
        }
    }
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
addImagePatch(const std::string& patchName)
{
	//create a new image patch
	ImagePatch<PixelParam>* newIp = new ImagePatch<PixelParam>(patchName);
	//store the new image patch:
	patches.push_back(newIp);
}

template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
update()
{
    int depth = updateFinestLevels();
    updateCoarserLevels(depth);
}

///\todo remove
#define VERIFYQUADTREEFILE 1
template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
verifyQuadtreeNode(Node* node)
{
    typename Node::State::File::TileIndex childIndices[4];
    assert(node->treeState->file->readTile(node->tileIndex, childIndices));

    if (node->children == NULL)
    {
        for (uint i=0; i<4; ++i)
            assert(childIndices[i] == Node::State::File::INVALID_TILEINDEX);
        return;
    }
    for (uint i=0; i<4; ++i)
        assert(childIndices[i] == node->children[i].tileIndex);
    for (uint i=0; i<4; ++i)
        verifyQuadtreeNode(&node->children[i]);
}
template <typename PixelParam, typename PolyhedronParam>
void Builder<PixelParam, PolyhedronParam>::
verifyQuadtreeFile(Node* node)
{
#if VERIFYQUADTREEFILE
    //go up to parent
    while (node->parent != NULL)
        node = node->parent;
    
    return verifyQuadtreeNode(node);
#endif //VERIFYQUADTREEFILE
}

END_CRUSTA
