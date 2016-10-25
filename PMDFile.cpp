/** ____________________________________________________________________________

\file PMDFile.cpp

\brief 
PMDFile class method

\author Programmer: Mathieu Lobet
\date Creation:   Fri Oct 14 2016

\warning READ BEFORE MODIFY:
\n This file should be modified/maintained only when located in its original repository.
\n Else, this file is a copy and may not be the lastest version.
\n The modifications will not be considered.

 ____________________________________________________________________________ */

#include "PMDFile.h"

/** ____________________________________________________________________________
 Method: PMDFile::PMDFile

 \brief Constructor: Initialize the object

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
PMDFile::PMDFile()
{
	verbose=0;
	fileId=-1;
	strcpy(this->version,"");
}

/** ____________________________________________________________________________
 Method: PMDFile::~PMDFile

 \brief Destructor of the container PMDFile 

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
PMDFile::~PMDFile()
{

}

/** ____________________________________________________________________________
 Method: PMDFile::OpenFile

 \brief Open the OpenPMD file

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::OpenFile(char * PMDFilePath)
{
    hid_t fileAccessPropListID = H5Pcreate(H5P_FILE_ACCESS);

    herr_t err = H5Pset_fclose_degree(fileAccessPropListID, H5F_CLOSE_SEMI);

    // Open the file
	fileId = H5Fopen(PMDFilePath, H5F_ACC_RDONLY, H5P_DEFAULT);

	H5Pclose(fileAccessPropListID);

	// The path is copied in this->filePath if the file was well opened.
	strcpy(this->filePath,PMDFilePath);
}

/** ____________________________________________________________________________
 Method: PMDFile::ScanFileAttributes

 \brief This method scans all attributes at the root of the file

 \author Programmer: Mathieu Lobet
 \date Creation:   Tue Oct 25 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::ScanFileAttributes()
{
	int 			iAttr;
    char 			attrName[64];
	hsize_t			nbAttr;
	hid_t			groupId;
    hid_t			attrId;
    hid_t 			atype;
	hid_t 			aspace;

	// OpenPMD files always contain a data group at the root
	groupId = H5Gopen(fileId, "/",H5P_DEFAULT);

	// Number of attributes
	nbAttr = H5Aget_num_attrs(groupId);

	if (verbose) cout << "    Number of file attributes: " << nbAttr << endl;

	// Loop over the attributes
    for (iAttr = 0; iAttr < nbAttr; iAttr++)
    {
    	// Opening of the attribute
		attrId = H5Aopen_idx(groupId, (unsigned int)iAttr );

		// Get the name of the attribute
		H5Aget_name(attrId, 64, attrName);
		/* the dimensions of the attribute data */
		aspace = H5Aget_space(attrId);
		// The type of the attribute 
		atype  = H5Aget_type(attrId);

		if (strcmp(attrName,"openPMD")==0)
		{
			// Read attribute
			H5Aread (attrId, atype, this->version);
		}

    }

}

/** ____________________________________________________________________________
 Method: PMDFile::ScanIterations

 \brief This method scans the group /data that contains the iteration groups.

 \details Each iteration is stored in the member iterations which is a vector of objects PMDIteration.
 Iteration group attributes are read and store too.

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::ScanIterations()
{
	hsize_t 		nbIterations; // Number of iterations
	hsize_t			nbAttr;
    hid_t    		groupId;
    hid_t			iterationId;
    hid_t			attrId;
    int 			i;
    int 			iAttr;
    int 			length;
    herr_t 			err;
    char			iterationName[64];
    char 			bufAttrName[64];
    PMDIteration 	iteration;
    double 			val;
    hid_t 			atype;
	hid_t 			aspace;
	H5O_info_t 		objectInfo;

	// OpenPMD files always contain a data group at the root
	groupId = H5Gopen(fileId, "/data",H5P_DEFAULT);

	//H5Gget_num_objs(group_iterations->getId(), &nobj);
	H5Gget_num_objs(groupId, &nbIterations);

	if (verbose) cout << "    Number of iterations: " << nbIterations << endl;

	// We scan by "hand" all groups in the group data that corresponds to the different iterations 

	// iteration over the iteration group
    for (i = 0; i < nbIterations; i++)
    {

		// Get the object name
		length = H5Gget_objname_by_idx(groupId, (hsize_t)i, 
			iterationName, (size_t) 64);

		// Get the type: group, dataset...
		err = H5Oget_info_by_name(groupId, iterationName , &objectInfo, H5P_DEFAULT);

		// Check that it is a group, we ignore dataset in the data group...
		if (objectInfo.type == H5O_TYPE_GROUP)
		{

		    // Openning of the iteration group
			iterationId = H5Gopen2(groupId, iterationName, H5P_DEFAULT);

			// Save the iteration name
			strcpy(iteration.name,iterationName);

			// Number of attributes
			nbAttr = H5Aget_num_attrs(iterationId);

			// Loop over the attributes
		    for (iAttr = 0; iAttr < nbAttr; iAttr++)
		    {
				attrId = H5Aopen_idx(iterationId, (unsigned int)iAttr );

				// Get the name of the attribute
				H5Aget_name(attrId, 64, bufAttrName);
				/* the dimensions of the attribute data */
				aspace = H5Aget_space(attrId);
				// The type of the attribute 
				atype  = H5Aget_type(attrId); 

				if (strcmp(bufAttrName,"dt")==0)
				{
					// Read attribute
					H5Aread (attrId, atype, &val);
					iteration.dt = val;
				}
				else if (strcmp(bufAttrName,"time")==0)
				{
					// Read attribute
					H5Aread (attrId, atype, &val);
					iteration.time = val;
				}
				else if (strcmp(bufAttrName,"timeUnitSI")==0)
				{
					// Read attribute
					H5Aread (attrId, atype, &val);
					iteration.timeUnitSI = val;
				}

				H5Aclose(attrId);
			}

			// Add the iteration in the list of iterations
			iterations.push_back(iteration);
		}
    }

    H5Gclose(groupId);

}

/** ____________________________________________________________________________
 Method: PMDFile::ScanFields

 \brief This method scans the fields in each iteration. For this aim, this method  
 calls the PMDIteration method ScanFields.

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::ScanFields()
{
	for (std::vector<PMDIteration>::iterator it = iterations.begin() ; it != iterations.end(); ++it)
	{
	 	it->ScanFields(this->fileId);
	}
}

/** ____________________________________________________________________________
 Method: PMDFile::ScanParticles

 \brief This method scans the particles in each iteration. For this aim, this method calls the 
 PMDIteration method ScanParticles.

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::ScanParticles()
{
	for (std::vector<PMDIteration>::iterator it = iterations.begin() ; it != iterations.end(); ++it)
	{
	 	it->ScanParticles(this->fileId);
	}
}

/** ____________________________________________________________________________
 Method: PMDFile::Print

 \brief This method prints the structure of the file.

 \details For this aim, this method calls the 
 PMDIteration method PMDIteration::PrintInfo().

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::Print()
{

	cout << " File: " << this->filePath << endl;
	cout << " OpenPMD Version: " << this->version << endl;

	cout << endl;
	cout << " Number of iteration: " << GetNumberIterations() << endl;
	for (std::vector<PMDIteration>::iterator it = iterations.begin() ; it != iterations.end(); ++it)
	{
	 	it->PrintInfo();
	}
}

/** ____________________________________________________________________________
 Method: PMDFile::GetNumberIterations

 \brief This method gives the number of iteration in the opened file.

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
int PMDFile::GetNumberIterations()
{
	return iterations.size();
}

/** ____________________________________________________________________________
 Method: PMDFile::CloseFile

 \brief This method closes the opened OpenPMD file.

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
void PMDFile::CloseFile()
{
	H5Fclose(fileId);
}

/** ____________________________________________________________________________
 Method: PMDFile::ReadScalarDataSet

 \brief This method reads the specified scalar dataset given by path 
 and returns the resulting array.

 \details
 This methods returns an error integer code equal to 0 if success.

 \param array pointer to the array that will receive the content of the dataset
 \param numValues number of element to read
 \param dataSetClass type of element to be read (H5T_FLOAT, H5T_INTEGER...)
 \param path path to the data set in the OpenPMD file

 \author Programmer: Mathieu Lobet
 \date Creation:   Fri Oct 14 2016

 Modifications:

 ____________________________________________________________________________ */
int PMDFile::ReadScalarDataSet(void * array,int numValues,H5T_class_t dataSetClass,char * path)
{

	int 	ndims;
    hid_t   datasetId;
    hid_t   datasetType;
    hid_t   datasetSpace;
    hsize_t datasetStorageSize;

    // Open the corresponding dataset
    if ((datasetId = H5Dopen(this->fileId,path,H5P_DEFAULT))<0)
    {
        //char error[1024];
        //SNPRINTF(error, 1024, "Problem when opening the dataset %d",int(datasetId));
        //EXCEPTION2(InvalidFilesException, (const char *)filename,error);
        cerr << "Problem when opening the dataset: " << path << endl;
        return -1;
    }  
    else
    {

        // Data space
        datasetSpace = H5Dget_space(datasetId);
        // Data type
        datasetType  = H5Dget_type(datasetId);
        // Storage size
        datasetStorageSize = H5Dget_storage_size(datasetId);
        // Dimension from the data space
        ndims        = H5Sget_simple_extent_ndims(datasetSpace);  

        // Check the class of the dataset
        if ((H5Tget_class(datasetType) == dataSetClass)&&(dataSetClass==H5T_FLOAT))
        {
            // Correct number of values in the dataset
            if (numValues == int(datasetStorageSize/4))
            {

                if (H5Dread(datasetId, datasetType, H5S_ALL, H5S_ALL, H5P_DEFAULT, array) < 0)
                {
                    //EXCEPTION1(InvalidVariableException, field->name);
                    cerr << "Problem when reading the dataset: " << path << endl;
                    return -4;
                }

            }
            else
            {
                //char error[1024];
                //SNPRINTF(error, 1024, "Invalid size for the current dataset (%d %ld)",numValues,long(datasetStorageSize));
                //EXCEPTION2(InvalidFilesException, (const char *)filename,error);
            	cerr << "Invalid size for the current dataset:" << numValues << " " << long(datasetStorageSize) << endl;
                return -3;
            }        	
        }
        else
        {
            //EXCEPTION2(InvalidFilesException, (const char *)filename,
            //               "The current dataset is not a float dataset");
            cerr << "The current dataset, " << path << ", is not of the specified class:" << H5T_FLOAT << endl;
            return -2;
        }

        H5Dclose(datasetId);
        H5Sclose(datasetSpace);
        H5Tclose(datasetType); 

    }
   	return 0;
}