/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for WRFChem data         	                   **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Leyla Kern                                                  **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  31.07.2019                                                      **
\**************************************************************************/


#include "ReadWRFChem.h"

#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>

#include <boost/filesystem.hpp>
#include <netcdf>

using namespace vistle;
namespace bf = boost::filesystem;


ReadWRFChem::ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm)
    : vistle::Reader ("Read WRF Chem data files", name, moduleID, comm)
{
    ncFirstFile = NULL;

    m_gridOut = createOutputPort("grid_out", "grid");
    m_filedir = addStringParameter("file_dir", "NC files directory","/mnt/raid/home/hpcleker/Desktop/test_files/NC/test_time", Parameter::Directory);

    m_numPartitionsLat = addIntParameter("num_partitions_lat", "number of partitions in lateral", 1);
    m_numPartitionsVer = addIntParameter("num_partitions_ver", "number of partitions in vertical", 1);

    m_varDim = addStringParameter("var_dim","Dimension of variables","",Parameter::Choice);
    setParameterChoices(m_varDim, varDimList);

    m_trueHGT = addStringParameter("true_height", "Use real ground topology", "", Parameter::Choice);
    m_gridLat = addStringParameter("GridX", "grid Sout-North axis", "", Parameter::Choice);
    m_gridLon = addStringParameter("GridY", "grid East_West axis", "", Parameter::Choice);
    m_PH = addStringParameter("pert_gp","perturbation geopotential", "", Parameter::Choice);
    m_PHB = addStringParameter("base_gp", "base-state geopotential", "", Parameter::Choice);
    m_gridZ = addStringParameter("GridZ", "grid Bottom-Top axis","", Parameter::Choice);

    char namebuf[50];
    std::vector<std::string> varChoices;
    varChoices.push_back("(NONE)");

    for (int i = 0; i < NUMPARAMS-3; ++i) {

        sprintf(namebuf, "Variable%d", i);

        std::stringstream s_var;
        s_var << "Variable" << i;
        m_variables[i] = addStringParameter(s_var.str(), s_var.str(), "", Parameter::Choice);

        setParameterChoices(m_variables[i], varChoices);
        observeParameter(m_variables[i]);
        s_var.str("");
        s_var << "data_out" << i;
        m_dataOut[i] = createOutputPort(s_var.str(), "scalar data");

    }
    m_variables[NUMPARAMS-3] = addStringParameter("U","U", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS-3], varChoices);
    observeParameter(m_variables[NUMPARAMS-3]);
    m_dataOut[NUMPARAMS-3] = createOutputPort("data_out_U", "scalar data");
    m_variables[NUMPARAMS-2] = addStringParameter("V","V", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS-2], varChoices);
    observeParameter(m_variables[NUMPARAMS-2]);
    m_dataOut[NUMPARAMS-2] = createOutputPort("data_out_V", "scalar data");
    m_variables[NUMPARAMS-1] = addStringParameter("W","W", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS-1], varChoices);
    observeParameter(m_variables[NUMPARAMS-1]);
    m_dataOut[NUMPARAMS-1] = createOutputPort("data_out_W", "scalar data");

    setParameterChoices(m_gridLat, varChoices);
    setParameterChoices(m_gridLon, varChoices);
    setParameterChoices(m_trueHGT, varChoices);
    setParameterChoices(m_PH, varChoices);
    setParameterChoices(m_PHB, varChoices);
    setParameterChoices(m_gridZ, varChoices);

    setParallelizationMode(Serial);
    setAllowTimestepDistribution(true);

    observeParameter(m_filedir);
    observeParameter(m_varDim);
}


ReadWRFChem::~ReadWRFChem() {
    if (ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
}


// inspectDir: check validity of path and create list of files in directory
bool ReadWRFChem::inspectDir() {
    //TODO :: check if file is NC format!
    std::string sFileDir = m_filedir->getValue();

     if (sFileDir.empty()) {
        sendInfo("WRFChem filename is empty!");
        return false;
    }

     try {
         bf::path dir(sFileDir);
         fileList.clear();
         numFiles = 0;

         if (bf::is_directory(dir)) {
             sendInfo("Locating files in %s", dir.string().c_str());
             for (bf::directory_iterator it(dir) ; it != bf::directory_iterator(); ++it) {
                 if (bf::is_regular_file(it->path()) &&(bf::extension(it->path().filename())==".nc")) {
                     // std::string fName = it->path().filename().string();
                     std::string fPath = it->path().string();
                     fileList.push_back(fPath);
                     ++numFiles;
                 }
             }
         }else if (bf::is_regular_file(dir)) {
             if (bf::extension(dir.filename())==".nc") {
                 std::string fName = dir.string();
                 sendInfo("Loading file %s", fName.c_str());
                 fileList.push_back(fName);
                 ++numFiles;
             }else {
                 sendError("File does not end with '.nc' ");
             }
         }else {
             sendInfo("Could not find given directory. Please specify a valid path");
             return false;
         }
     } catch (std::exception &ex) {
         sendError("Could not read %s: %s", sFileDir.c_str(), ex.what());
         return false;
     }

    if (numFiles > 1) {
        std::sort(fileList.begin(), fileList.end(), [](std::string a, std::string b) {return a<b;}) ;
    }
    sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
            return false;
    return true;
}

bool ReadWRFChem::prepareRead() {
    if (!ncFirstFile) {
        ReadWRFChem::examine(nullptr);
    }

 /*   if (ncFirstFile->is_valid()) {
        for (int vi=0; vi<NUMPARAMS; ++vi) {
            std::string name = "";
            int nDim = 0;
            NcToken refDim[4]={"","","",""};

            name = m_variables[vi]->getValue();
            if ((name != "") && (name != "NONE")) {
                NcVar* var = ncFirstFile->get_var(name.c_str());
                nDim = var->num_dims();
                if (strcmp(refDim[0],"")!=0) {
                    for(int di=0; di<nDim;++di) {
                         NcDim *dim = var->get_dim(di);
                         if (dim->name() != refDim[di]) {
                             sendInfo("Variables rely on different dimensions. Please select matching variables");
                             return false;
                         }
                    }
                }else {
                    for(int di=0; di<nDim;++di) {
                        refDim[di]=var->get_dim(di)->name();
                    }
                }
            }
        }
    }
*/
    int N_p = static_cast<int>( m_numPartitionsLat->getValue() * m_numPartitionsLat->getValue() * m_numPartitionsVer->getValue());
    setPartitions(N_p);
   return true;
}


bool ReadWRFChem::examine(const vistle::Parameter *param) {

    if (!param || param == m_filedir || param == m_varDim) {
       if (!inspectDir())
           return false;
       sendInfo("File %s is used as base", fileList.front().c_str());

       if (ncFirstFile) {
           delete ncFirstFile;
           ncFirstFile = nullptr;
       }
       std::string sDir =/* m_filedir->getValue() + "/" + */fileList.front();
       try {
            ncFirstFile = new NcFile(sDir.c_str(), NcFile::read);


            std::vector<std::string> AxisChoices;
            std::vector<std::string> Axis2dChoices;

            AxisChoices.clear();
            Axis2dChoices.clear();

            /*if (!ncFirstFile->is_valid()) {
                sendInfo("Failed to access file");
                return false;
            }*/
            int num3dVars = 0;
            int num2dVars = 0;
            int nVar = ncFirstFile->getVarCount();

            char *newEntry = new char[50];
            bool is_fav = false;
            std::vector<std::string> favVars = {"co","no2","PM10","o3","U","V","W"};

            
            std::multimap<std::string, NcVar> allVars = ncFirstFile->getVars();
            for (const auto& var : allVars)
            {
                std::string newEntry= var.second.getName();

                if (var.second.getDimCount() > 3) {
                    for (std::string fav : favVars) {
                        if (newEntry == fav) {
                            AxisChoices.insert(AxisChoices.begin(), newEntry);
                            is_fav = true;
                            break;
                        }
                    }
                    if (is_fav == false)
                        AxisChoices.push_back(newEntry);
                    num3dVars++;
                    is_fav = false;
                }else if (var.second.getDimCount() > 2) {
                    for (std::string fav : favVars) {
                        if (newEntry== fav) {
                            Axis2dChoices.insert(Axis2dChoices.begin(), newEntry);
                            break;
                        }
                    }
                    if (is_fav == false)
                        Axis2dChoices.push_back(newEntry);
                    num2dVars++;
                    is_fav = false;
                }
            }

            AxisChoices.insert(AxisChoices.begin(),"NONE");
            Axis2dChoices.insert(Axis2dChoices.begin(),"NONE");

            if (m_varDim->getValue() == varDimList[1]) {//3D
                sendInfo("Variable dimension: 3D");
                for (int i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], AxisChoices);
            }else if (m_varDim->getValue() == varDimList[0]) { //2D
                sendInfo("Variable dimension: 2D");
                for (int i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], Axis2dChoices);
            }else {
                sendInfo("Please select the dimension of variables");
            }
            setParameterChoices(m_trueHGT, Axis2dChoices);
            setParameterChoices(m_gridLat, Axis2dChoices);
            setParameterChoices(m_gridLon, Axis2dChoices);
            setParameterChoices(m_PHB, AxisChoices);
            setParameterChoices(m_PH, AxisChoices);
            setParameterChoices(m_gridZ, AxisChoices);

            setTimesteps(numFiles);

        } 
        catch(...)
        {
            sendError( "Could not open NC file" );
            return false;
        }
    }

    return true;
}

//setMeta: set the meta data
void ReadWRFChem::setMeta(Object::ptr obj, int blockNr, int totalBlockNr, int timestep) const {
    if(!obj)
        return;

    obj->setTimestep(timestep);
    //obj->setNumTimesteps(numFiles);
    obj->setRealTime(0);

    obj->setBlock(blockNr);
    obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);
}

//emptyValue: check if variable selction is empty
bool ReadWRFChem::emptyValue(vistle::StringParameter *ch) const {
    std::string name = "";
    name = ch->getValue();
    if ((name == "") || (name=="NONE"))
        return true;
    else return false;
}

//computeBlock: compute indices of current block and ghost cells
ReadWRFChem::Block ReadWRFChem::computeBlock(int part, int nBlocks, long blockBegin, long cellsPerBlock, long numCellTot) const {
    int partIdx = blockBegin/cellsPerBlock;
    int begin = blockBegin, end = blockBegin + cellsPerBlock + 1;
    int numGhostBeg = 0, numGhostEnd = 0;

    if (begin > 0) {
        --begin;
        numGhostBeg = 1;
    }

    if (partIdx == nBlocks - 1) {
        end = numCellTot;
    }else if (end < numCellTot - 1) {
        ++end;
        numGhostEnd = 1;
    }

    Block block;
    block.part = part;
    block.begin = begin;
    block.end = end;
    block.ghost[0] = numGhostBeg;
    block.ghost[1] = numGhostEnd;

    return block;
}

//generateGrid: set grid coordinates for block b and attach ghosts
Object::ptr ReadWRFChem::generateGrid(Block *b) const {
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;
    Object::ptr geoOut;

    if(!emptyValue(m_gridLat) && !emptyValue(m_gridLon) && ((!emptyValue(m_PH) && !emptyValue(m_PHB)) || !emptyValue(m_gridZ) )) {
        //use geographic coordinates
        StructuredGrid::ptr strGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
        auto ptrOnXcoords = strGrid->x().data();
        auto ptrOnYcoords = strGrid->y().data();
        auto ptrOnZcoords = strGrid->z().data();

       // float * hgt = new float[bSizeY*bSizeZ];
        float * lat = new float[bSizeY*bSizeZ];
        float * lon = new float[bSizeY*bSizeZ];
        NcVar varLat = ncFirstFile->getVar(m_gridLat->getValue());
        NcVar varLon = ncFirstFile->getVar(m_gridLon->getValue());
        std::vector<NcDim> dims;
        std::vector<NcDim> dimsPH;
        std::vector<NcDim> dimsPHB;
      //  NcVar *varHGT = ncFirstFile->get_var(m_trueHGT->getValue().c_str());
        //extract (2D) lat, lon, hgt
       // varHGT->set_cur(0,b[1].begin,b[2].begin);
       // varHGT->get(hgt, 1,bSizeY, bSizeZ);
        std::vector<size_t> start{0,size_t(b[1].begin),size_t(b[2].begin)};
        std::vector<size_t> size{ 1,size_t(bSizeY)    ,size_t(bSizeZ)};
        varLat.getVar(start,size,lat);
        varLon.getVar(start,size,lon);

        if (!emptyValue(m_gridZ)) {
            float * z = new float[(bSizeX+1)*bSizeY*bSizeZ];
            NcVar varZ = ncFirstFile->getVar(m_gridZ->getValue());
            int numDimElem = varZ.getDimCount();
            dims = varZ.getDims();

            std::vector<size_t> start{0,size_t(b[0].begin),size_t(b[1].begin),size_t(b[2].begin)};
            std::vector<size_t> size{ 1,size_t(bSizeX)    ,size_t(bSizeY)    ,size_t(bSizeZ)};
            /*curs[numDimElem-3] = b[0].begin;
            curs[numDimElem-2] = b[1].begin;
            curs[numDimElem-1] = b[2].begin;
            curs[0] = 0;

            numElem[numDimElem-3] = bSizeX+1;
            numElem[numDimElem-2] = bSizeY;
            numElem[numDimElem-1] = bSizeZ;*/

            varZ.getVar(start,size,z);

            int n = 0;
            int idx1 = 0;
            for (int i = 0; i < bSizeX; i++) {
                for (int j = 0; j < bSizeY; j++) {
                    for (int k = 0; k < bSizeZ; k++, n++) {
                        idx1 = (i+1)*bSizeY*bSizeZ + j*bSizeZ + k;
                        ptrOnXcoords[n] = z[idx1];
                        ptrOnYcoords[n] = lat[j*bSizeZ+k];
                        ptrOnZcoords[n] = lon[j*bSizeZ+k];
                    }
                }
            }
            delete [] z;

        } else if (!emptyValue(m_PH) &&!emptyValue(m_PHB)) {
            float * ph = new float[(bSizeX+1)*bSizeY*bSizeZ];
            float * phb = new float[(bSizeX+1)*bSizeY*bSizeZ];
            NcVar varPH = ncFirstFile->getVar(m_PH->getValue());
            NcVar varPHB = ncFirstFile->getVar(m_PHB->getValue());

            //extract (3D) geopotential for z-coord calculation
            dimsPH = varPH.getDims();
            dimsPHB = varPHB.getDims();

            std::vector<size_t> start{0,size_t(b[0].begin),size_t(b[1].begin),size_t(b[2].begin)};
            std::vector<size_t> size{ 1,size_t(bSizeX)    ,size_t(bSizeY)    ,size_t(bSizeZ)};
            int numDimElem = varPH.getDimCount();

            varPH.getVar(start,size,ph);
            varPHB.getVar(start,size,phb);

            //geopotential height is defined on stagged grid -> one additional layer
            //thus it is evaluated (vertically) inbetween vertices to match lat/lon grid
            int n = 0;
            int idx = 0, idx1 = 0;
            for (int i = 0; i < bSizeX; i++) {
                for (int j = 0; j < bSizeY; j++) {
                    for (int k = 0; k < bSizeZ; k++, n++) {
                        idx = i*bSizeY*bSizeZ + j*bSizeZ + k;
                        idx1 = (i+1)*bSizeY*bSizeZ + j*bSizeZ + k;
                        ptrOnXcoords[n] = (ph[idx]+phb[idx]+ph[idx1]+phb[idx1])/(2*9.81);
                        ptrOnYcoords[n] = lat[j*bSizeZ+k];
                        ptrOnZcoords[n] = lon[j*bSizeZ+k];
                    }
                }
            }
            delete [] ph;
            delete [] phb;
        }

        for (int i=0; i<3; ++i) {
            strGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0]);
            strGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }

        strGrid->updateInternals();
       // delete [] hgt;
        delete [] lat;
        delete [] lon;

        geoOut = strGrid;
    }else if (!emptyValue(m_trueHGT)) {
        //use terrain height
        StructuredGrid::ptr strGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
        auto ptrOnXcoords = strGrid->x().data();
        auto ptrOnYcoords = strGrid->y().data();
        auto ptrOnZcoords = strGrid->z().data();

        NcVar varHGT = ncFirstFile->getVar(m_trueHGT->getValue());
        float * hgt = new float[bSizeY*bSizeZ];

        std::vector<NcDim> dims;
        dims = varHGT.getDims();
        std::vector<size_t> start{0,size_t(b[1].begin),size_t(b[2].begin)};
        std::vector<size_t> size{ 1,size_t(bSizeY)    ,size_t(bSizeZ)};
        varHGT.getVar(start,size,hgt);

        int n = 0;
        for (int i = 0; i < bSizeX; i++) {
            for (int j = 0; j < bSizeY; j++) {
                for (int k = 0; k < bSizeZ; k++, n++) {
                    ptrOnXcoords[n] = (i+b[0].begin+hgt[k+bSizeZ*j]/50);  //divide by 50m (=dx of grid cell)
                    ptrOnYcoords[n] = j+b[1].begin;
                    ptrOnZcoords[n] = k+b[2].begin;
                }
            }
        }
        for (int i=0; i<3; ++i) {
            strGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0]);
            strGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }
        strGrid->updateInternals();
        geoOut = strGrid;

        delete [] hgt;
    }else {
        //uniform coordinates
        UniformGrid::ptr uniGrid(new UniformGrid(bSizeX, bSizeY, bSizeZ));

        for (unsigned i = 0; i < 3; ++i) {
            uniGrid->min()[i] = b[i].begin;
            uniGrid->max()[i] = b[i].end;

            uniGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0] );
            uniGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }
        uniGrid->updateInternals();
        geoOut = uniGrid;
    }

    return geoOut;

}



//addDataToPort: read and set values for variable and add them to the output port
bool ReadWRFChem::addDataToPort(Token &token, NcFile *ncDataFile, int vi, Object::ptr outGrid, Block *b, int block, int t) const {

    if (!(StructuredGrid::as(outGrid) || UniformGrid::as(outGrid)))
        return true;
    NcVar varData = ncDataFile->getVar(m_variables[vi]->getValue());
    std::string unit;
    varData.getAtt("units").getValues(unit);
    int numDimElem = varData.getDimCount();
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;

    std::vector<size_t> start{0,size_t(b[0].begin),size_t(b[1].begin),size_t(b[2].begin)};
    std::vector<size_t> size{ 1,size_t(bSizeX)    ,size_t(bSizeY)    ,size_t(bSizeZ)};
    //int numDimElem = varPH.getDimCount();


    Vec<Scalar>::ptr obj(new Vec<Scalar>(bSizeX*bSizeY*bSizeZ));
    vistle::Scalar *ptrOnScalarData = obj->x().data();

    if ((vi==NUMPARAMS-1) && (numDimElem >3)) { //W has one level to many: -> read and crop
         size[1] = bSizeX+1;
         float * longdata = new float[(bSizeX+1)*bSizeY*bSizeZ];
         varData.getVar(start,size,longdata);
         int n = 0;
         int idx1 = 0;
         for (int i = 1; i < bSizeX+1; i++) {
             for (int j = 0; j < bSizeY; j++) {
                 for (int k = 0; k < bSizeZ; k++, n++) {
                     idx1 = (i)*bSizeY*bSizeZ + j*bSizeZ + k;
                     ptrOnScalarData[n] =longdata[idx1] ;
                 }
             }
         }
        delete [] longdata;
    }else {
         varData.getVar(start,size,ptrOnScalarData);
    }
    obj->setGrid(outGrid);
    setMeta(obj, block, numBlocks, t);
    obj->setMapping(DataBase::Vertex);
    std::string pVar = m_variables[vi]->getValue();
    obj->addAttribute("_species", pVar+ " [" + unit + "]");
    token.addObject(m_dataOut[vi], obj);

    return true;
}


bool ReadWRFChem::read(Token &token, int timestep, int block) {
    int numBlocksLat = m_numPartitionsLat->getValue();
    int numBlocksVer = m_numPartitionsVer->getValue();
    if ((numBlocksLat <= 0) || (numBlocksVer <= 0)) {
        sendInfo("Number of partitions cannot be zero!");
        return false;
    }
    numBlocks = numBlocksLat*numBlocksLat*numBlocksVer;


            NcVar var;
            int numdims = 0;
            std::vector<NcDim> dims;

            //TODO: number of dimensions can be set by any variable, when check in prepareRead is used to ensure matching dimensions of all variables
            for (int vi = 0; vi < NUMPARAMS; vi++) {
               std::string name = "";
               name = m_variables[vi]->getValue();
               if ((name != "") && (name != "NONE")) {
                    var = ncFirstFile->getVar(name);
                    if (var.getDimCount() > numdims) {
                        numdims = var.getDimCount();
                        dims = var.getDims();
                    }
                }
            }

            if (numdims == 0) {
                sendInfo("Failed to load variables: Dimension is zero");
                return false;
            }

            int nx = dims[numdims - 3].getSize() /*Bottom_Top*/, ny = dims[numdims - 2].getSize() /*South_North*/, nz = dims[numdims - 1].getSize()/*East_West*/;//, nTime = edges[0] /*TIME*/ ;
            long blockSizeVer = (nx)/numBlocksVer, blockSizeLat = (ny)/numBlocksLat;
            if (numdims <= 3) {
                nx = 1;
                blockSizeVer = 1;
                numBlocksVer = 1;
            }

            //set offsets for current block
            int blockXbegin = block /(numBlocksLat*numBlocksLat) * blockSizeVer;  //vertical direction (Bottom_Top)
            int blockYbegin = ((static_cast<long>((block % (numBlocksLat*numBlocksLat)) / numBlocksLat)) * blockSizeLat);
            int blockZbegin = (block % numBlocksLat)*blockSizeLat;

            Block b[3];

            b[0] = computeBlock(block, numBlocksVer, blockXbegin, blockSizeVer, nx );
            b[1] = computeBlock(block, numBlocksLat, blockYbegin, blockSizeLat, ny );
            b[2] = computeBlock(block, numBlocksLat, blockZbegin, blockSizeLat, nz );

            if (!outObject[block]) {
                //********* GRID *************
               outObject[block] = generateGrid(b);
               setMeta(outObject[block], block, numBlocks, -1);
            }
            if (timestep == -1) {
               token.addObject(m_gridOut, outObject[block]);
            }else {
                // ******** DATA *************
                std::string sDir = /*m_filedir->getValue() + "/" + */ fileList.at(timestep);
                
                try {
                    NcFile *ncDataFile = new NcFile(sDir.c_str(), NcFile::read);
                    for (int vi = 0; vi < NUMPARAMS; ++vi)
		    {
                        if (emptyValue(m_variables[vi])) {
                            continue;
                        }
                        addDataToPort(token, ncDataFile, vi, outObject[block], b,  block, timestep);
                    }
                delete ncDataFile;
                }
                catch(...)
                {
                    sendError("Could not open data file at time %i", timestep);
                    return false;
                }

            }
            return true;
}

bool ReadWRFChem::finishRead() {
    if(ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
    return true;
}


MODULE_MAIN(ReadWRFChem)