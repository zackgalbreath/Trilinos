/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */        
/* ******************************************************************** */

/************************************************************************/
/*          Utilities for Trilinos/ML users                             */
/*----------------------------------------------------------------------*/
/* Authors : Mike Heroux (SNL)                                          */
/*           Jonathan Hu  (SNL)                                         */
/*           Ray Tuminaro (SNL)                                         */
/*           Marzio Sala (SNL)                                          */
/************************************************************************/


#include "ml_common.h"
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS)

#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_FECrsMatrix.h"
#include "Epetra_VbrMatrix.h"
#include "Epetra_SerialDenseMatrix.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_SerialDenseSolver.h"
#include "Epetra_Import.h"
#include "Epetra_Time.h"
#include "Epetra_Operator.h"
#include "Epetra_RowMatrix.h"
#ifdef ML_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

//#include <cstring>
#include "ml_amesos_wrap.h"
#include "ml_ifpack_wrap.h"
#include "ml_agg_METIS.h"
#include "ml_epetra_utils.h"

#include "ml_epetra_preconditioner.h"
#include "ml_agg_ParMETIS.h"

#include "ml_anasazi.h"

#include "ml_ggb.h"

using namespace Teuchos;
using namespace ML_Epetra;

extern "C" {
  extern int ML_ggb_Set_SymmetricCycle(int flag);
  extern int ML_ggb_Set_CoarseSolver(int flag);
}

// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::SetFiltering() 
{

  int ReturnValue = 0;
  
  char parameter[80];
  Epetra_Time Time(Comm());

  string Pre(Prefix_);
  
  sprintf(parameter,"%sfiltering: enable", Prefix_);
  if( ! List_.get(parameter,false) ) return -1;
  
    
  sprintf(parameter,"%sfiltering: use symmetric cycle", Prefix_);
  bool FltUseSym = List_.get(parameter,false);
  if( FltUseSym == true ) ML_ggb_Set_SymmetricCycle(1);
  else                    ML_ggb_Set_SymmetricCycle(0);

  int restarts = List_.get(Pre+"eigen-analysis: restart", 50);
  int NumEigenvalues = List_.get(Pre+"filtering: eigenvalues to compute", 5);
  int length = List_.get(Pre+"eigen-analysis: length", NumEigenvalues);
  int BlockSize = List_.get(Pre+"eigen-analysis: block-size", 1);
  double tol = List_.get(Pre+"eigen-analysis: tolerance", 1e-5);

  if( length <= NumEigenvalues ) length = NumEigenvalues+1;
  
  string Eigensolver = List_.get(Pre+"filtering: eigensolver", "Anasazi");

  if( verbose_ ) {
    cout << endl;
    cout << PrintMsg_ << "\tFiltering the preconditioner: computing low-convergent modes..." << endl;
    cout << PrintMsg_ << "\t- scheme = " << Eigensolver << endl;
    cout << PrintMsg_ << "\t- number of eigenvectors to compute = " << NumEigenvalues << endl;
    cout << PrintMsg_ << "\t- tolerance = " << tol << endl;
    cout << PrintMsg_ << "\t- block size = " << BlockSize << endl;
    cout << PrintMsg_ << "\t- length     = " << length << endl;
    if( FltUseSym ) cout << PrintMsg_ << "\t- using symmetric preconditioner" << endl;
    else            cout << PrintMsg_ << "\t- using a non-symmetric preconditioner" << endl;
      
  }    
    
  if( Eigensolver =="ARPACK" ) {

    // check, only 1 proc is supported
    if( Comm().NumProc() != 1 ) {
      if( Comm().MyPID() == 0 )
	cerr << ErrorMsg_ << "ARPACK can be used only for serial run" << endl;
      exit( EXIT_FAILURE );
    }

    ML_ggb_Set_CoarseSolver(2); // set Amesos KLU as solver

    flt_MatrixData_ = new(struct ML_CSR_MSRdata);
      
    // stick values in Haim's format
    struct ML_Eigenvalue_Struct ml_eig_struct;
    ml_eig_struct.Max_Iter = restarts;
    ml_eig_struct.Num_Eigenvalues = NumEigenvalues;
    ml_eig_struct.Arnoldi = length;
    ml_eig_struct.Residual_Tol = tol;

#ifdef HAVE_ML_ARPACK
    ML_ARPACK_GGB(&ml_eig_struct, ml_, flt_MatrixData_, 0, 0);
#else
    cerr << ErrorMsg_ << "You must configure ML with --with-ml_arpack to use" << endl
	 << ErrorMsg_ << "ARPACK as eigensolver." << endl
	 << ErrorMsg_ << "Try `filtering: eigensolver' = `Anasazi' instead." << endl;
#endif
  
    // now build the correction to be added to the ML cycle
      
    ML_build_ggb(ml_, flt_MatrixData_);
      
  } else if( Eigensolver == "Anasazi" ) {
      
    // 1.- set parameters for Anasazi
    Teuchos::ParameterList AnasaziList;
    AnasaziList.set("eigen-analysis: matrix operation", "I-ML^{-1}A");
    AnasaziList.set("eigen-analysis: use diagonal scaling", false);
    AnasaziList.set("eigen-analysis: symmetric problem", false);
    AnasaziList.set("eigen-analysis: length", length);
    AnasaziList.set("eigen-analysis: block-size", BlockSize);
    AnasaziList.set("eigen-analysis: tolerance", tol);
    AnasaziList.set("eigen-analysis: action", "LM");
    AnasaziList.set("eigen-analysis: restart", restarts);
    AnasaziList.set("eigen-analysis: output", 0);

    // data to hold real and imag for eigenvalues and eigenvectors
    double * RealEigenvalues = new double[NumEigenvalues];
    double * ImagEigenvalues = new double[NumEigenvalues];

    double * RealEigenvectors = new double[NumEigenvalues*NumMyRows()];
    double * ImagEigenvectors = new double[NumEigenvalues*NumMyRows()];

    if( RealEigenvectors == 0 || ImagEigenvectors == 0 ) {
      cerr << ErrorMsg_ << "Not enough space to allocate "
	   << NumEigenvalues*NumMyRows()*8 << " bytes for filtering/Anasazi" << endl;
      exit( EXIT_FAILURE );
    }
      
    // this is the starting value -- random
    Epetra_MultiVector EigenVectors(OperatorDomainMap(),NumEigenvalues);
    EigenVectors.Random();

    int NumRealEigenvectors, NumImagEigenvectors;

#ifdef HAVE_ML_ANASAZI
    // 2.- call Anasazi and store the results in eigenvectors      
    ML_Anasazi::Interface(RowMatrix_,EigenVectors,RealEigenvalues,
			  ImagEigenvalues, AnasaziList, RealEigenvectors,
			  ImagEigenvectors,
			  &NumRealEigenvectors, &NumImagEigenvectors, ml_);
#else
    if( Comm().MyPID() == 0 ) {
      cerr << ErrorMsg_ << "ML has been configure without the Anasazi interface" << endl
	   << ErrorMsg_ << "You must add the option --enable-anasazi to use" << endl
	   << ErrorMsg_ << "filtering and Anasazi" << endl;
    }
    exit( EXIT_FAILURE );
#endif

    // small matrices may turn out crazy. Warn the user that his code is
    // essentially broken, consider re-employment or pre-retirement.
    // Stop coding is also highly suggested.

    if( NumRealEigenvectors+NumImagEigenvectors == 0 ) {
      cerr << ErrorMsg_ << "Anasazi has computed no nonzero eigenvalues" << endl
	   << ErrorMsg_ << "This sometimes happen because your fine grid matrix" << endl
	   << ErrorMsg_ << "is too small. In this case, try to change the Anasazi input" << endl
	   << ErrorMsg_ << "parameters, or drop the filtering correction." << endl;
      exit( EXIT_FAILURE );
    }
      
    // 3.- some output, to print out how many vectors we are actually using
    if( verbose_ ) {
      cout << PrintMsg_ << "\t- Computed eigenvalues of I - ML^{-1}A:" << endl;
      for( int i=0 ; i<NumEigenvalues ; ++i ) {
	cout << PrintMsg_ << "\t  z = " << std::setw(10) << RealEigenvalues[i]
	     << " + i(" << std::setw(10) << ImagEigenvalues[i] << " ),  |z| = "
	     << sqrt(pow(RealEigenvalues[i],2.0) + pow(ImagEigenvalues[i],2)) << endl;
      }
      cout << PrintMsg_ << "\t- Using " << NumRealEigenvectors << " real and "
	   << NumImagEigenvectors << " imaginary eigenvector(s)" << endl;
    }

    int size = NumRealEigenvectors+NumImagEigenvectors;
      
    assert( size<2*NumEigenvalues+1 );

    string PrecType = List_.get(Pre+"filtering: type", "enhanced");
      
    if( PrecType == "projection" ) {
	
      // 4.- build the restriction operator as a collection of vectors
      flt_R_ = new Epetra_MultiVector(Map(),size);
      assert( flt_R_ != 0 );
	
      for( int i=0 ; i<NumRealEigenvectors ; ++i ) {
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  (*flt_R_)[i][j] = RealEigenvectors[j+i*NumMyRows()];
      }
	
      for( int i=0 ; i<NumImagEigenvectors ; ++i ) {
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  (*flt_R_)[NumRealEigenvectors+i][j] = ImagEigenvectors[j+i*NumMyRows()];
      }
	
      //FIXME      Epetra_MultiVector AQ(Map(),size);
      //FIXME AQ.PutScalar(0.0);
	
      //FIXME      RowMatrix_->Multiply(false,*flt_R_, AQ);
	
      // 5.- epetra linear problem for dense matrices
      flt_rhs_.Reshape(size,1);
      flt_lhs_.Reshape(size,1);
      flt_A_.Reshape(size,size);
      flt_solver_.SetVectors(flt_lhs_, flt_rhs_);
      flt_solver_.SetMatrix(flt_A_);
	
      // 6.- build the "coarse" matrix as a serial matrix, replicated
      //     over all processes. LAPACK will be used for its solution
      for( int i=0 ; i<size ; ++i ) {
	for( int j=0 ; j<size ; ++j ) {
	  double value;
	  (*flt_R_)(i)->Dot(*((*flt_R_)(j)), &value); // FIXME : it was AQ(j)
	  flt_A_(i,j) = value;
	}
      }
	
      // compute the inverse, overwrite the old values of A
      flt_solver_.Invert();
	
    } else if( PrecType == "ml-cycle" ) {

      if( Comm().NumProc() != 1 ) {
	cerr << ErrorMsg_ << "Option `filtering: type' == `ml-cycle' can be used only with 1 process." << endl
	     << ErrorMsg_ << "You can select `projection' or `enhanced' for multi-process runs" << endl;
	exit( EXIT_FAILURE );
      }

      ML_ggb_Set_CoarseSolver(2); // set Amesos KLU as solver
	
      // Here I use the Haim's code. Note that is works with one process only,
      // but it allows comparisons between ARPACK and Anasazi

      // a.- copy the null space in a double vector, as now I have real
      //     and imag null space in two different vectors (that may be only
      //     partially populated)

      flt_NullSpace_ = new double[(NumRealEigenvectors+NumImagEigenvectors)*NumMyRows()];
      if( flt_NullSpace_ == 0 ) {
	cerr << ErrorMsg_ << "Not enough space to allocate "
	     << (NumRealEigenvectors+NumImagEigenvectors)*NumMyRows()*8 << " bytes" << endl;
	exit( EXIT_FAILURE );
      }

      int count = 0;
      for( int i=0 ; i<NumRealEigenvectors ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  flt_NullSpace_[count++] = RealEigenvectors[j+i*NumMyRows()];

      for( int i=0 ; i<NumImagEigenvectors ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  flt_NullSpace_[count++] = ImagEigenvectors[j+i*NumMyRows()];

      flt_MatrixData_ = new(struct ML_CSR_MSRdata);

      ML_GGB2CSR(flt_NullSpace_,NumRealEigenvectors+NumImagEigenvectors,
		 NumMyRows(), flt_MatrixData_, 0);
	
      // now build the correction to be added to the ML cycle
	
      ML_build_ggb(ml_, flt_MatrixData_);
	
    } else if( PrecType == "enhanced" ) {

      // this is equivalent to the "fattening" of Haim. I build a new ML
      // hierarchy, using the null space just computed. I have at least one
      // aggregate per subdomain, using METIS.

      // a.- copy the null space in a double vector, as now I have real
      //     and imag null space in two different vectors (that may be only
      //     partially populated)

      flt_NullSpace_ = new double[(NumRealEigenvectors+NumImagEigenvectors)*NumMyRows()];
      if( flt_NullSpace_ == 0 ) {
	cerr << ErrorMsg_ << "Not enough space to allocate "
	     << (NumRealEigenvectors+NumImagEigenvectors)*NumMyRows()*8 << " bytes" << endl;
	exit( EXIT_FAILURE );
      }

      int count = 0;
      for( int i=0 ; i<NumRealEigenvectors ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  flt_NullSpace_[count++] = RealEigenvectors[j+i*NumMyRows()];

      for( int i=0 ; i<NumImagEigenvectors ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  flt_NullSpace_[count++] = ImagEigenvectors[j+i*NumMyRows()];
	
      int NumAggr = List_.get(Pre+"filtering: local aggregates",1);

      // b.- create a new ML hierarchy, whose null space has been computed
      //     with the iteration matrix modes

      if( verbose_ ) cout << endl << "Building ML hierarchy for filtering" << endl << endl;
      
      ML_Create(&flt_ml_,2); // now only 2-level methods
      ML_Operator_halfClone_Init( &(flt_ml_->Amat[1]),
				  &(ml_->Amat[ml_->ML_finest_level]));
			      
      ML_Aggregate_Create(&flt_agg_);
      ML_Aggregate_Set_CoarsenScheme_METIS(flt_agg_);
      ML_Aggregate_Set_LocalNumber(flt_ml_,flt_agg_,-1,NumAggr);
      ML_Aggregate_Set_DampingFactor(flt_agg_,0.0);
      ML_Aggregate_Set_Threshold(flt_agg_, 0.0);
      ML_Aggregate_Set_MaxCoarseSize(flt_agg_, 1);
      ML_Aggregate_Set_NullSpace(flt_agg_, NumPDEEqns_,NumRealEigenvectors+NumImagEigenvectors,
				 flt_NullSpace_, NumMyRows());
      int CoarsestLevel = ML_Gen_MultiLevelHierarchy_UsingAggregation(flt_ml_,1, ML_DECREASING, flt_agg_);

      ML_Gen_Smoother_Amesos(flt_ml_, 0, ML_AMESOS_KLU, -1);
      ML_Gen_Solver(flt_ml_, ML_MGV, 1, 0);
   
      ml_->void_options = (void *) flt_ml_;

      if( verbose_ ) cout << endl;
      
    } else if( PrecType == "let ML be my master" ) {

      int dim = NumPDEEqns_+NumRealEigenvectors+NumImagEigenvectors;
      NullSpaceToFree_ = new double[dim*NumMyRows()];
      if( NullSpaceToFree_ == 0 ) {
	cerr << ErrorMsg_ << "Not enough space to allocate "
	     << dim*NumMyRows()*8 << " bytes" << endl;
	exit( EXIT_FAILURE );
      }

      // default null-space
      for( int i=0 ; i<NumPDEEqns_ ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j )
	  if( j%NumPDEEqns_ == i ) NullSpaceToFree_[j+i*NumMyRows()] = 1.0;
	  else                     NullSpaceToFree_[j+i*NumMyRows()] = 0.0;

      int count = NumPDEEqns_*NumMyRows();

      for( int i=0 ; i<NumRealEigenvectors ; ++i )
	for( int j=0 ; j<NumMyRows() ; ++j ) 
	  NullSpaceToFree_[count++] = RealEigenvectors[j+i*NumMyRows()];

      ReturnValue = dim;
      
    } else {

      if( Comm().MyPID() == 0 ) {
	cerr << ErrorMsg_ << "Value of option `filtering: type' not correct" << endl
	     << ErrorMsg_ << "(" << PrecType << ") It should be: " << endl
	     << ErrorMsg_ << "<projection> / <ml-cycle> / <enhanced>" << endl;
      }
      exit( EXIT_FAILURE );
    }
      
    // 7.- free some junk
    delete [] RealEigenvalues;
    delete [] ImagEigenvalues;
    delete [] RealEigenvectors;
    delete [] ImagEigenvectors;
	
  } else {

    if(  Comm().MyPID() == 0 )
      cerr << ErrorMsg_ << "Value of option `filtering:eigensolver' not correct" << endl
	   << ErrorMsg_ << "(" << Eigensolver << "). It should be:" << endl
	   << ErrorMsg_ << "<ARPACK> / <Anasazi>" << endl;

    exit( EXIT_FAILURE );
  }

  if( verbose_ ) 
    cout << PrintMsg_ << "\t- Total Time for filtering setup = " << Time.ElapsedTime() << " (s)" << endl;
    
  return( ReturnValue );
  
}

#endif /*ifdef ML_WITH_EPETRA && ML_HAVE_TEUCHOS*/
