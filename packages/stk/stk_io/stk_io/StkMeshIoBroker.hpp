/*------------------------------------------------------------------------*/
/*  Copyright 2010, 2011, 2012, 2013 Sandia Corporation.                  */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef STK_IO_STKMESHIOBROKER_HPP
#define STK_IO_STKMESHIOBROKER_HPP
#include <string>
#include <vector>
#include <boost/any.hpp>

#include <Ioss_PropertyManager.h>
#include <Ioss_Field.h>
#include <Teuchos_RCP.hpp>

#include <stk_util/parallel/Parallel.hpp>
#include <stk_util/util/ParameterList.hpp>

#include <stk_mesh/base/CoordinateSystems.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/ConnectivityMap.hpp>

#include <stk_io/IossBridge.hpp>
#include <stk_io/DatabasePurpose.hpp>

namespace Ioss {
  class DatabaseIO;
  class Region;
}

namespace stk {
  namespace mesh {
    class Part;
    class BulkData;
    class Selector;
  }
  namespace io {
    static std::string CoordinateFieldName("coordinates");

    class OutputFile
    {
    public:
        OutputFile(const std::string &filename, MPI_Comm communicator, DatabasePurpose db_type,
		   Ioss::PropertyManager& property_manager, const Ioss::Region *input_region)
        : m_current_output_step(-1), m_use_nodeset_for_part_nodes_fields(false),
          m_mesh_defined(false), m_fields_defined(false), m_db_purpose(db_type),
	  m_input_region(input_region), m_subset_selector(NULL)
        {
          setup_output_file(filename, communicator, property_manager);
        }

        OutputFile(Teuchos::RCP<Ioss::Region> ioss_output_region, MPI_Comm communicator,
		   DatabasePurpose db_type, const Ioss::Region *input_region)
        : m_current_output_step(-1), m_use_nodeset_for_part_nodes_fields(false),
          m_mesh_defined(false), m_fields_defined(false), m_db_purpose(db_type),
	  m_input_region(input_region), m_subset_selector(NULL)
        {
            m_output_region = ioss_output_region;
            m_mesh_defined = true;
        }
        Teuchos::RCP<Ioss::Region> get_output_io_region() {
            return m_output_region;
        }
        ~OutputFile()
        {
          stk::io::delete_selector_property(*m_output_region);
	}

        void write_output_mesh(const stk::mesh::BulkData& bulk_data);
        void add_field(stk::mesh::FieldBase &field, const std::string &alternate_name);

        void add_global(const std::string &variableName, const boost::any &value, stk::util::ParameterType::Type type);
        void add_global(const std::string &variableName, Ioss::Field::BasicType dataType);
        void add_global(const std::string &variableName, const std::string &type, Ioss::Field::BasicType dataType);
        void add_global(const std::string &variableName, int component_count,     Ioss::Field::BasicType dataType);

        void write_global(const std::string &variableName,
			  const boost::any &value, stk::util::ParameterType::Type type);
        void write_global(const std::string &variableName, double globalVarData);
        void write_global(const std::string &variableName, int globalVarData);
        void write_global(const std::string &variableName, std::vector<double>& globalVarData);
        void write_global(const std::string &variableName, std::vector<int>& globalVarData);

        void begin_output_step(double time, const stk::mesh::BulkData& bulk_data);
        void end_output_step();

        int write_defined_output_fields(const stk::mesh::BulkData& bulk_data);
        int process_output_request(double time, const stk::mesh::BulkData& bulk_data);

        void set_subset_selector(Teuchos::RCP<stk::mesh::Selector> my_selector);

        bool use_nodeset_for_part_nodes_fields() const
        {
	  return m_use_nodeset_for_part_nodes_fields;
        }

        void use_nodeset_for_part_nodes_fields(bool true_false)
        {
	  ThrowErrorMsgIf(m_mesh_defined,
			  "ERROR: The use_nodeset_for_part_nodes_fields setting cannot be changed after "
			  "the mesh has already been written.");
	  m_use_nodeset_for_part_nodes_fields = true_false;
        }

    private:
        void define_output_fields(const stk::mesh::BulkData& bulk_data);
        void setup_output_file(const std::string &filename, MPI_Comm communicator,
			       Ioss::PropertyManager &property_manager);

        int m_current_output_step;
        bool m_use_nodeset_for_part_nodes_fields;
        bool m_mesh_defined;
        bool m_fields_defined;
        DatabasePurpose m_db_purpose;
        const Ioss::Region* m_input_region;
        Teuchos::RCP<stk::mesh::Selector> m_subset_selector;
        Teuchos::RCP<Ioss::Region> m_output_region;
        std::vector<stk::io::FieldAndName> m_named_fields;

        OutputFile(const OutputFile &);
        const OutputFile & operator=(const OutputFile &);
    };

    // ========================================================================    
    enum HeartbeatType {
      BINARY = 1, /* Exodus (history file) */
      CSV,        /* Comma-seperated values */
      TS_CSV,     /* same as CSV except lines preceded by timestamp*/
      TEXT,       /* Same as CSV except fields separated by tab (by default) */
      TS_TEXT,    /* same as TEXT except lines preceded by timestamp*/
      SPYHIS
    };

    // ------------------------------------------------------------------------
    struct HeartbeatVariable {
      HeartbeatVariable(const std::string &name, boost::any *value, stk::util::ParameterType::Type type)
	: m_name(name), m_value(value), m_type(type)
      {}

      std::string m_name;
      boost::any *m_value;
      stk::util::ParameterType::Type m_type;
    };

    // ------------------------------------------------------------------------
    class Heartbeat {
    public:
      Heartbeat(const std::string &filename, HeartbeatType db_type,
		Ioss::PropertyManager properties, MPI_Comm comm);
      ~Heartbeat() {};
      
      void add_global(const std::string &variableName, boost::any &value, stk::util::ParameterType::Type type);
      void process_output(int step, double time);

    private:
      std::vector<HeartbeatVariable> m_fields;
      Teuchos::RCP<Ioss::Region> m_region;
      
      int m_current_step;
      int m_processor;
    };

    // ========================================================================    

    class StkMeshIoBroker {
      public:

        /**
         * \param[in] comm MPI Communicator to be used for all
         * parallel communication needed to generate the mesh.
         */
        StkMeshIoBroker(MPI_Comm comm, stk::mesh::ConnectivityMap * connectivity_map = NULL);
        StkMeshIoBroker();

        ~StkMeshIoBroker();

        /**
	 * Add the specified 'new_property' to the default property
	 * manager for this StkMeshIoBroker object.  The property
	 * manager is (currently) used for the input mesh region and
	 * the output mesh regions if no output property manager is
	 * specified in the create_output_mesh() call.
	 *
	 * If the property already exists, it is overwritten by the
	 * current value.
	 */	 
        void property_add(const Ioss::Property &property);

        /**
	 * If a property with the specified name exists, remove it.
	 * (Function provided for backward compatibility with existing
	 * use in Percept following change to make m_property_manger
	 * private)
	 */
        void remove_property_if_exists(const std::string &property_name);

        /**
         * Set the input Ioss::Region directly instead of letting it be
         * created by StkMeshIoBroker during the create_input_mesh(type,
         * filename) call. After setting the input io region, you would
         * then either set the metadata manually using the
         * set_meta_data() call, or call the no-argument
         * create_input_mesh() function which will then create a meta
         * data corresponding to the data in the Ioss::Region.
	 *
	 * [2013-11-13: GDS: Currently
         * only used in Salinas/tools/superelem/MkSuperStkMesh.C:
         * The use-case is adding new parts to a mesh]
         */
        void set_input_io_region(Teuchos::RCP<Ioss::Region> ioss_input_region);

        Teuchos::RCP<Ioss::Region> get_input_io_region()
        {
          if (Teuchos::is_null(m_input_region) && !Teuchos::is_null(m_input_database)) {
            create_ioss_region();
          }
          return m_input_region;
        }

        Teuchos::RCP<Ioss::Region> get_output_io_region(size_t output_file_index) {
            validate_output_file_index(output_file_index);
            return m_output_files[output_file_index]->get_output_io_region();
        }

        /**
         * The default entity rank names are "NODE", "EDGE", "FACE", "ELEMENT"
         * If an application wants to change these names and/or add additional
         * names, it should call this function with a vector containing the
         * desired names.  To use the default names, but add additional names,
         * call stk::mesh::entity_rank_names() which will return a reference
         * to the default entity name vector; copy these and add the new names
         * and then call this function.
         *
         * Must be called prior to create_input_mesh() or it will have no affect
         *
         * @param rank_names -- std::vector<std::string> containing the names
         * of your entities.
         */
        void set_rank_name_vector(const std::vector<std::string> &rank_names);

        /**
         * The selector is an optional selector that can be provided
         * which will be used to filter the mesh entities (node,
         * element, face, edge) associated with an
         * Ioss Entity (element block, node block, node set, ...). The
         * optional selector will be 'anded' with the normal selector
         * (typically locally owned part) used to associate entities
         * when generating the output database.
         */
        void set_subset_selector(size_t output_file_index, Teuchos::RCP<stk::mesh::Selector> my_selector) {
	    validate_output_file_index(output_file_index);
            m_output_files[output_file_index]->set_subset_selector(my_selector);
	}

        void set_subset_selector(size_t output_file_index, stk::mesh::Selector &my_selector) {
	  validate_output_file_index(output_file_index);
	  m_output_files[output_file_index]->set_subset_selector(Teuchos::rcpFromRef(my_selector));
	}

        Teuchos::RCP<stk::mesh::Selector> deprecated_selector() {
           return m_deprecated_selector;
	}
        void deprecated_set_selector(Teuchos::RCP<stk::mesh::Selector> my_selector) {
	  m_deprecated_selector = my_selector;
	}

        /**
         * Set bulk data directly with your own bulk data. If this is
         * not called prior to the populate_bulk_data() call, it will be
         * created automatically using the communicator of the m_input_region.
         * If meta data is not already set, then set the meta data from the
         * bulk data's metadata
         */
        void set_bulk_data(Teuchos::RCP<stk::mesh::BulkData> arg_bulk_data);
        void set_bulk_data(stk::mesh::BulkData &arg_bulk_data)
        { set_bulk_data(Teuchos::rcpFromRef(arg_bulk_data));}

        /**
         * Create the Ioss::DatabaseIO associated with the specified filename
         * and type (exodus by default). The routine checks that the
         * file exists and is readable and will return false if not.
         * No meta_data or bulk_data is created at this time, but the
         * get_input_io_database() will be valid.
         *
         * The Ioss::DatabaseIO can be accessed via the get_input_io_database()
         * method if you need to set some options on the database prior
         * to it being read.
         *
         * \param[in] filename If the mesh type is file based ("exodus"),
         * then this contains the full pathname to the file containing the
         * mesh information.  If the mesh type is a generated type, then
         * this parameter contains data used by the generation routines.
         * See the GeneratedMesh documentation.
         *
         * \param[in] type The format of the mesh that will be "read".
         * Valid types are "exodus", "generated", "pamgen".
         *
         * \param[in] type The format of the mesh that will be "read".
         * Valid types are "exodus", "generated", "pamgen".
         *
         * \param[in] purpose The purpose (READ_MESH or READ_RESTART) of the mesh database.
	 * An input of type READ_RESTART will read all-but-one of the states of a multi-state field,
	 * an input of type READ_MESH will only read the newest state of a multi-state field.
	 * Other behavioral differences may be added in the future (e.g., dealing with adaptivity...)
	 */
        bool open_mesh_database(std::string filename,
                                const std::string &type, DatabasePurpose purpose);

        /**
         * Create Ioss::DatabaseIO associated with the specified filename using
         * the default filetype (typically "exodus"). If the filename is
         * prepended with a type followed by a colon (e.g., "generated:10x10x10"),
         * then use that type.  Valid input types are:
         *   exodus, dof, pamgen, and possibly others.
         *
         * \param[in] filename If the mesh type is file based ("exodus"),
         * then this contains the full pathname to the file containing the
         * mesh information.  If the mesh type is a generated type, then
         * this parameter contains data used by the generation routines.
         * Optionally prepended by a filetype and a colon.
         */
        bool open_mesh_database(std::string filename, DatabasePurpose db_type);

        /**
         * Read/Generate the metadata for mesh of the specified type. By
         * default, all entities in the mesh (nodeblocks, element blocks,
         * nodesets, sidesets) will have an associated stk mesh part
         * created for it.
         *
         * If m_input_region is non-null then this is assumed to be a
         * valid Ioss::Region* that should be used instead of opening
         * the file and creating a new Ioss::Region. This can be set via
         * the set_input_io_region(Ioss::Region *region) call prior to
         * calling this function.
         *
         * Following this call, the 'populate_bulk_data()' function should
         * be called to read the bulk data from the mesh and generate the
         * corresponding stk mesh entities (nodes, elements, faces, ...)
         *
         * The meta_data will not be committed by this function, so the
         * caller will need to call meta_data.commit() after the
         * function returns.
         */
        void create_input_mesh();

        /**
         * Read/Generate the bulk data for the mesh.  This function will
         * create all stk mesh entities (nodes, elements) with the
         * correct nodeal coordinates, element connectivity, element
         * attribute data, and nodeset and sideset membership.  Note
         * that meta_data.commit() needs to be called prior to calling
         * this function.
         * Note: this function internally calls the two methods
         * populate_mesh() and populate_field_data(), declared below,
         * and does NOT do the delayed field-data allocation optimization.
         */
        void populate_bulk_data();

        /**
         * Read/generate the bulk data for the mesh, delaying (by default)
         * the allocation of any field-data such as coordinates, attributes
         * and distribution factors.
         * This field-data is read/generated by separately calling the
         * populate_field_data() method declared below.
         * Note that the above-declared 'populate_bulk_data()' method
         * calls both of these methods.
         */
        void populate_mesh(bool delay_field_data_allocation = true);

        /**
         * Read/generate the field-data for the mesh, including
         * coordinates, attributes and distribution factors.
         * Note that this call should be preceded by a call to
         * the above-declared populate_mesh() method.
         */
        void populate_field_data();

        /**
         * For all transient fields on the mesh database:
	 * - declare a stk_field of the same name,
	 * - add the field as in input field for the mesh (call add_input_field()).
	 *
	 * Then, when read_defined_input_fields() is called, the data on the mesh will
	 * be read and populate the corresponding stk field.
	 *
	 * NOTE: This should only be called if you want *ALL* transient
	 * fields on the input mesh to be defined as stk fields and read.
         */
        void add_all_mesh_fields_as_input_fields();

        /**
         * For all transient input fields defined, read the data at the
         * specified database step 'step' (1-based) and populate the stk
         * data structures with those values.
         */
        double read_defined_input_fields(int step);

        /**
         * For all transient input fields defined, read the data at the
         * specified database time 'time' and populate the stk
         * data structures with those values.  The database time closest
         * to the specified time will be used with no interpolation (yet).
         */
        double read_defined_input_fields(double time);

        void get_global_variable_names(std::vector<std::string> &names);
        void get_global(const std::string &variableName,
			boost::any &value, stk::util::ParameterType::Type type);
        void get_global(const std::string &variableName, int &globalVar);
        void get_global(const std::string &variableName, double &globalVar);
        void get_global(const std::string &variableName, std::vector<double> &globalVar);
        void get_global(const std::string &variableName, std::vector<int> &globalVar);

        void add_input_field(stk::mesh::FieldBase &field, const std::string &db_name = std::string());

        /**
         * Create an exodus mesh database with the specified
         * filename. This function creates the exodus metadata which
         * is the number and type of element blocks, nodesets, and
         * sidesets; and then outputs the mesh bulk data such as the
         * node coordinates, id maps, element connectivity.  When the
         * function returns, the non-transient portion of the mesh will
         * have been defined.
         *
         * A stk part will have a corresponding exodus entity (element
         * block, nodeset, sideset) defined if the "is_io_part()" function
         * returns true.  By default, all parts read from the mesh
         * database in the create_input_mesh() function will return true
         * as will all stk parts on which the function
         * stk::io::put_io_part_attribute() was called.  The function
         * stk::io::remove_io_part_attribute(part) can be called to omit a
         * part from being output.
         *
         * \param[in] filename The full pathname to the file which will be
         * created and the mesh data written to. If the file already
         * exists, it will be overwritten.
	 *
         * \param[in] purpose The purpose (WRITE_RESULTS or WRITE_RESTART) of the output mesh.
	 * An output of type WRITE_RESTART will write all-but-one of the states of a multi-state field,
	 * an output of type WRITE_RESULTS will only write the newest state of a multi-state field.
	 * Other behavioral differences may be added in the future (e.g., dealing with adaptivity...)
         */
        size_t create_output_mesh(const std::string &filename, DatabasePurpose purpose);
        size_t create_output_mesh(const std::string &filename, DatabasePurpose purpose, Ioss::PropertyManager &properties);

        void write_output_mesh(size_t output_file_index);

        void add_field(size_t output_file_index, stk::mesh::FieldBase &field);
        void add_field(size_t output_file_index, stk::mesh::FieldBase &field, const std::string &db_name);

        void add_global(size_t output_file_index, const std::string &variableName,
			const boost::any &value, stk::util::ParameterType::Type type);
        void add_global(size_t output_file_index, const std::string &variableName,
			Ioss::Field::BasicType dataType);
        void add_global(size_t output_file_index, const std::string &variableName,
			const std::string &type, Ioss::Field::BasicType dataType);
        void add_global(size_t output_file_index, const std::string &variableName,
			int component_count,     Ioss::Field::BasicType dataType);

        /**
         * Add a transient step to the database at time 'time'.
         */
        void begin_output_step(size_t output_file_index, double time);
        void end_output_step(size_t output_file_index);

        /**
         * Output the data for all defined fields to the database for
	 * the step added by "begin_output_step".  End step with a call
	 * to "end_output_step"
         */
        int write_defined_output_fields(size_t output_file_index);

        /**
         * Add a transient step to the mesh database at time 'time' and
         * output the data for all defined fields to the database.
	 * Performs the same functions as:
	 *
	 * 	begin_output_step(output_file_index, time);
	 * 	write_defined_output_fields(output_file_index);
	 * 	end_output_step(output_file_index);
	 *
	 * Note that if there are any global varibles defined, they
	 * will *not* be output with this function and will be
	 * zero-filled on the database.
         */
        int process_output_request(size_t output_file_index, double time);

        void write_global(size_t output_file_index, const std::string &variableName,
			  const boost::any &value, stk::util::ParameterType::Type type);
        void write_global(size_t output_file_index, const std::string &variableName, double data);
        void write_global(size_t output_file_index, const std::string &variableName, int data);
        void write_global(size_t output_file_index, const std::string &variableName, std::vector<double>& data);
        void write_global(size_t output_file_index, const std::string &variableName, std::vector<int>& data);

        // ========================================================================
        // Add a history or heartbeat output...
        size_t add_heartbeat_output(const std::string &filename, HeartbeatType db_type,
				    const Ioss::PropertyManager &properties = Ioss::PropertyManager());
  
        void add_heartbeat_global(size_t index, const std::string &name,
				  boost::any &value, stk::util::ParameterType::Type type)
        {
	  ThrowRequire(index < m_heartbeat.size());
	  m_heartbeat[index]->add_global(name, value, type);
        }
  
        void process_heartbeat_output(size_t index, int step, double time)
        {
	  ThrowRequire(index < m_heartbeat.size());
	  m_heartbeat[index]->process_output(step, time);
        }
  
        // ========================================================================
        bool is_meta_data_null() const
        {
          return Teuchos::is_null(m_meta_data);
        }

        bool is_bulk_data_null() const
        {
          return Teuchos::is_null(m_bulk_data);
        }

        stk::mesh::MetaData &meta_data()
        {
          ThrowRequire( !Teuchos::is_null(m_meta_data)) ;
          return *m_meta_data;
        }

        stk::mesh::BulkData &bulk_data()
        {
          ThrowRequire( !Teuchos::is_null(m_bulk_data)) ;
          return *m_bulk_data;
        }

        const stk::mesh::MetaData &meta_data() const
        {
          ThrowRequire( !Teuchos::is_null(m_meta_data)) ;
          return *m_meta_data;
        }

        const stk::mesh::BulkData &bulk_data() const
        {
          ThrowRequire( !Teuchos::is_null(m_bulk_data)) ;
          return *m_bulk_data;
        }

        /*!
         * Return the coordinate field for this mesh.
         */
        stk::mesh::FieldBase const& get_coordinate_field();

        /*!
         * If there are nodal fields defined on parts with higher-rank. For example, a nodal
         * field on all the nodes of the elements in an element block, then there are two
         * ways to output that field:
         * 1. Create a nodeset of those nodes and output the field on that nodeset
         * 2. Output the fields as nodal fields (on all nodes in the model; zero-fill if not defined on a node)
         *
         * The first method is the most efficient; however, many visualization packages
         * do not handle nodeset fields as well as they handle nodal field.  This function
         * specifies how the user application wants the fields output.  Set it via a call
         * to use_nodeset_for_part_nodes_fields(true_false);
         * @return
         */
        bool use_nodeset_for_part_nodes_fields(size_t output_file_index) const
        {
	  validate_output_file_index(output_file_index);
	  return m_output_files[output_file_index]->use_nodeset_for_part_nodes_fields();
        }
        void use_nodeset_for_part_nodes_fields(size_t output_file_index, bool true_false)
        {
	  validate_output_file_index(output_file_index);
	  m_output_files[output_file_index]->use_nodeset_for_part_nodes_fields(true_false);
        }

      private:
        void create_ioss_region();
        void validate_output_file_index(size_t output_file_index) const;

        /*!
         * The `m_property_manager` member data contains properties that
         * can be used to set database-specific options in the
         * Ioss::DatabaseIO class.  Examples include compression, name
         * lengths, integer sizes, floating point sizes. By convention,
         * the property name is all uppercase. Some existing properties
         * recognized by the Exodus Ioex::DatabaseIO class are:
         *
         * | Property              | Value
         * |-----------------------|-------------------
         * | COMPRESSION_LEVEL     | In the range [0..9]. A value of 0 indicates no compression
         * | COMPRESSION_SHUFFLE   | (true/false) to enable/disable hdf5's shuffle compression algorithm.
         * | FILE_TYPE             | netcdf4
         * | MAXIMUM_NAME_LENGTH   | Maximum length of names that will be returned/passed via api call.
         * | INTEGER_SIZE_DB       | 4 or 8 indicating byte size of integers stored on the database.
         * | INTEGER_SIZE_API      | 4 or 8 indicating byte size of integers used in api functions.
         * | LOGGING               | (true/false) to enable/disable logging of field input/output
         */
        Ioss::PropertyManager m_property_manager;

        MPI_Comm m_communicator;
        std::vector<std::string>       m_rank_names; // Optional rank name vector.

        Teuchos::RCP<Ioss::DatabaseIO> m_input_database;
        Teuchos::RCP<Ioss::Region>     m_input_region;

        Teuchos::RCP<stk::mesh::MetaData>  m_meta_data;
        Teuchos::RCP<stk::mesh::BulkData>  m_bulk_data;

        Teuchos::RCP<stk::mesh::Selector> m_deprecated_selector;

        stk::mesh::ConnectivityMap* m_connectivity_map;

        std::vector<Teuchos::RCP<OutputFile> > m_output_files;
        std::vector<Teuchos::RCP<Heartbeat> > m_heartbeat;

        std::vector<stk::io::FieldAndName> m_input_fields;

        DatabasePurpose m_db_purpose;

        StkMeshIoBroker(const StkMeshIoBroker&); // Do not implement
        StkMeshIoBroker& operator=(const StkMeshIoBroker&); // Do not implement
    };
  }
}
#endif
