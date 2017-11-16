/*
 * MueLu_IsorropiaInterface_decl.hpp
 *
 *  Created on: Jun 10, 2013
 *      Author: tobias
 */

#ifndef MUELU_ISORROPIAINTERFACE_DECL_HPP_
#define MUELU_ISORROPIAINTERFACE_DECL_HPP_

#include "MueLu_ConfigDefs.hpp"

//#if defined(HAVE_MUELU_ISORROPIA) && defined(HAVE_MPI)

#include <Xpetra_Matrix.hpp>
#include <Xpetra_MapFactory_fwd.hpp>
#include <Xpetra_BlockedMultiVector.hpp>
#include <Xpetra_BlockedVector.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_CrsGraphFactory.hpp> //TODO

#ifdef HAVE_MUELU_EPETRA
#include <Xpetra_EpetraCrsGraph.hpp>
#endif

#include "MueLu_SingleLevelFactoryBase.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Graph_fwd.hpp"
#include "MueLu_AmalgamationFactory_fwd.hpp"
#include "MueLu_AmalgamationInfo_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

/*!
  @class IsorropiaInterface
  @brief Interface to Isorropia
  @ingroup Rebalancing

  Interface to Isorropia allowing to access other rebalancing/repartitioning algorithms from Zoltan than RCB
  This includes methods (like PHG) which do not rely on user-provided coordinate or mesh information.
  This class produces node-based rebalancing information (stored in "AmalgamatedPartition") which is used as
  input for the RepartitionInterface class.

  It tries to consider the "number of partitions" variable when repartitioning the system.

  @note Only works with the Epetra stack in Xpetra

  ## Input/output of IsorropiaInterface ##

  ### User parameters of IsorropiaInterface ###
  Parameter | type | default | master.xml | validated | requested | description
  ----------|------|---------|:----------:|:---------:|:---------:|------------
  | A                                      | Factory | null  |   | * | * | Generating factory of the matrix A used during the prolongator smoothing process |
  | UnAmalgamationInfo | Factory |   null |  | * | * | Generating factory of UnAmalgamationInfo
  | number of partitions                   | GO      | - |  |  |  | Short-cut parameter set by RepartitionFactory. Avoid repartitioning algorithms if only one partition is necessary (see details below)

  The * in the @c master.xml column denotes that the parameter is defined in the @c master.xml file.<br>
  The * in the @c validated column means that the parameter is declared in the list of valid input parameters (see IsorropiaInterface::GetValidParameterList).<br>
  The * in the @c requested column states that the data is requested as input with all dependencies (see IsorropiaInterface::DeclareInput).

  ### Variables provided by IsorropiaInterface ###

  After IsorropiaInterface::Build the following data is available (if requested)

  Parameter | generated by | description
  ----------|--------------|------------
  | AmalgamatedPartition | IsorropiaInterface   | GOVector based on the node map associated with the graph of A

*/

  //FIXME: this class should not be templated
  template <class LocalOrdinal = int,
            class GlobalOrdinal = LocalOrdinal,
            class Node = KokkosClassic::DefaultNode::DefaultNodeType>
  class IsorropiaInterface : public SingleLevelFactoryBase {

    typedef double Scalar; // FIXME This class only works with the Epetra stack, i.e., Scalar = double
#undef MUELU_ISORROPIAINTERFACE_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    IsorropiaInterface() { }

    //! Destructor
    virtual ~IsorropiaInterface() { }
    //@}

    RCP<const ParameterList> GetValidParameterList() const;

    //! @name Input
    //@{
    void DeclareInput(Level & level) const;
    //@}

    //! @name Build methods.
    //@{
    void Build(Level &level) const;

    //@}



  private:



  };  //class IsorropiaInterface

} //namespace MueLu

#define MUELU_ISORROPIAINTERFACE_SHORT
//#endif //if defined(HAVE_MUELU_ISORROPIA) && defined(HAVE_MPI)


#endif /* MUELU_ISORROPIAINTERFACE_DECL_HPP_ */
