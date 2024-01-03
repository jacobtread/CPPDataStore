
#ifndef DATA_OBJECT
#define DATA_OBJECT 1

#include <fstream>
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

using std::ifstream;
using std::int32_t;
using std::map;
using std::ofstream;
using std::string;
using std::uint32_t;
using std::uint8_t;
using std::vector;

/// <summary>
/// Value stored within a DataObject, can be a String, Integer, or Float
/// </summary>
class DataValue {
 private:
  /// <summary>
  /// The type of data stored
  /// </summary>
  enum : uint8_t { STRING, INTEGER, FLOAT } type;
  union {
    /// <summary>
    /// String value
    /// </summary>
    string stringValue;
    /// <summary>
    /// Signed 32bit integer value
    /// </summary>
    int32_t intValue;
    /// <summary>
    /// Floating point value
    /// </summary>
    float floatValue;
  };

  /// <summary>
  /// Serializes this data value to the provided stream
  /// </summary>
  /// <param name="stream">The stream to write to</param>
  void serialize(std::ofstream& stream) const;

  /// <summary>
  /// Deserializes this data value from the provided stream
  /// </summary>
  /// <param name="stream">The stream to read from</param>
  void deserialize(std::ifstream& stream);

 public:
  /// <summary>
  /// Default constructor for creating data values, creates
  /// a INTEGER with the value of zero
  /// </summary>
  DataValue();

  /// <summary>
  /// Copy constructor for creating a data value from another
  /// data value
  /// </summary>
  DataValue(const DataValue& other);

  /// <summary>
  /// Destructor for handling the destruction logic of
  /// underlying types such a string
  /// </summary>
  ~DataValue();

  /// <summary>
  /// String constructor for creating a data value from a string
  /// </summary>
  DataValue(const string& value);

  /// <summary>
  /// Integer constructor for creating a data value from an integer
  /// </summary>
  DataValue(int32_t value);

  /// <summary>
  /// Integer constructor for creating a data value from a float
  /// </summary>
  DataValue(float value);

  /// <summary>
  /// Attempts to get a pointer to the underlying value as a string value,
  /// if the underlying value is not a string a nullptr is returned.
  /// </summary>
  /// <returns>Pointer to the string value or a nullptr</returns>
  string* asString();

  /// <summary>
  /// Attempts to get a pointer to the underlying value as a int value,
  /// if the underlying value is not a int a nullptr is returned.
  /// </summary>
  /// <returns>Pointer to the int value or a nullptr</returns>
  int32_t* asInt();

  /// <summary>
  /// Attempts to get a pointer to the underlying value as a float value,
  /// if the underlying value is not a float a nullptr is returned.
  /// </summary>
  /// <returns>Pointer to the float value or a nullptr</returns>
  float* asFloat();

  /// <summary>
  /// Assings self from the provided other data value
  /// </summary>
  DataValue& operator=(const DataValue& other);

  friend class DataObject;
};

/// <summary>
/// Object of data stored within a data object collection. Objects
/// contain a collection of key value entries.
///
/// Each object is uniquely identified by its ID field
/// </vsummary>
class DataObject {
 private:
  /// <summary>
  /// The Unique ID for this object
  /// </summary>
  uint32_t id;

  /// <summary>
  /// Collection of key value entries present in this object
  /// </summary>
  map<string, DataValue> entries;

  /// <summary>
  /// Deserializes the object from the provided stream
  /// </summary>
  /// <param name="stream">The stream to read from</param>
  void deserialize(std::ifstream& stream);

  /// <summary>
  /// Serializes the object writing it to the provided stream
  /// </summary>
  /// <param name="stream">The stream to write to</param>
  void serialize(std::ofstream& stream) const;

 public:
  /// <summary>
  /// Default constructor for creating an empty object
  /// </summary>
  DataObject();

  /// <summary>
  /// Provides the ID of this object
  /// </summary>
  /// <returns>The object ID</returns>
  uint32_t getId() const;

  /// <summary>
  /// Sets the entry at the provided key to the provided
  /// value
  /// </summary>
  /// <param name="key">The entry key</param>
  /// <param name="value">The entry value</param>
  void setEntry(string key, DataValue value);

  /// <summary>
  /// Provides the value for the key or a nullptr if the
  /// entry doesn't exist
  /// </summary>
  /// <param name="key">The entry key</param>
  DataValue* getEntry(string key);

  /// <summary>
  /// Clears the contents of the object
  /// </summary>
  void clear();

  friend class DataObjectCollection;
};

/// <summary>
/// Abstract class implemented by structures that can be
/// serialized and deserialized as DataObjects
/// </summary>
class DataObjectStructure {
 public:
  virtual ~DataObjectStructure() {}

  /// <summary>
  /// Provides the object ID for this structure if it was loaded
  /// from an existing object.
  ///
  /// Should be zero when storing the structure for the first time
  /// </summary>
  virtual uint32_t getObjectId() = 0;

  /// <summary>
  /// Populates the provided object with the values from this
  /// structure
  /// </summary>
  /// <param name="object">The object to populate</param>
  virtual void populateObject(DataObject* object) = 0;

  /// <summary>
  /// Creates this structure from the provided obejct
  /// </summary>
  /// <param name="object">The object to create from</param>
  virtual void fromObject(DataObject* object) = 0;
};

/// <summary>
/// Serializes the provided string value to the provided stream,
///
/// Used internally for serializing string values and entry keys
/// </summary>
/// <param name="stream">The stream to write to</param>
/// <param name="value">The string value to write</param>
void serializeString(ofstream& stream, const string& value);

/// <summary>
/// Deserializes a string from the provided stream, storing the
/// deserialized string in the provided out variable
/// </summary>
/// <param name="out">The string to store the value in</param>
void deserializeString(ifstream& stream, string& out);

/// <summary>
/// Collection of DataObjects creating a data store, this store can
/// load, save and creating new data objects from disk.
///
/// These can be loaded from data object files
/// </summary>
class DataObjectCollection {
 private:
  /// <summary>
  /// File path to where the collection is stored
  /// </summary>
  string path;
  /// <summary>
  /// Unique ID counter for the ID that should be given
  /// to the next object created
  /// </summary>
  uint32_t nextId;
  /// <summary>
  /// The underlying collection of objects
  /// </summary>
  vector<DataObject> objects;

 public:
  /// <summary>
  /// Creates a new data object collection for the provided path
  /// </summary>
  /// <param name="path">The path to the data object file</param>
  DataObjectCollection(string path);

  /// <summary>
  /// Deserializes this object collection from a file at the specific
  /// path for this colleciton.
  ///
  /// Will override any current objects and the nextId stored in this
  /// collection.
  ///
  /// If the file does not exist then the default state is applied.
  /// </summary>
  void load();

  /// <summary>
  /// Serializes this object collection saving it to the file at the
  /// provided path within this collection.
  ///
  /// Will create a new file if one does not exist. Will override
  /// any existing data present in the file
  /// </summary>
  void save() const;

  /// <summary>
  /// Provides a pointer to the object with the provided ID. If the
  /// obejct does not exist a nullptr is returned instead
  /// </summary>
  /// <param name="id">The ID of the object to return</param>
  /// <returns>The object with the provided ID or null</returns>
  DataObject* getObject(uint32_t id);

  /// <summary>
  /// Provides the total number of objects stored in this collection
  /// </summary>
  /// <returns>The number of objects</returns>
  size_t getObjectCount();

  /// <summary>
  /// Deletes an object with the provided ID if one is present
  ///
  /// Deleting an object saves the collection to disk
  /// </summary>
  /// <param name="id">The ID of the object to delete</param>
  void deleteObject(uint32_t id);

  /// <summary>
  /// Creates a new object, allocates the next ID
  /// to the object and increases the ID counter
  /// </summary>
  /// <returns>The newly allocated object</returns>
  DataObject* createObject();

  /// <summary>
  /// Stores the provided structure in object form within the collection.
  ///
  /// Saves the object collection automatically
  /// </summary>
  /// <returns>The object that was created and saved</returns>
  DataObject* storeStruct(DataObjectStructure* structure);

  /// <summary>
  /// Saves an existing data object structure back to the database with
  /// its new changes
  ///
  /// Saves the object collection automatically
  /// </summary>
  /// <returns>The underlying data object loaded from or nullptr if
  /// none</returns>
  DataObject* saveStruct(DataObjectStructure* structure);

  /// <summary>
  /// Loads an existing structure from the databse
  /// </summary>
  /// <returns>The underlying data object loaded from or nullptr if
  /// none</returns>
  DataObject* loadStruct(DataObjectStructure* structure);
};

#endif