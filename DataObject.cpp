#include "DataObject.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using std::ifstream;
using std::int32_t;
using std::ios;
using std::map;
using std::ofstream;
using std::string;
using std::uint32_t;
using std::vector;

DataObjectCollection::DataObjectCollection(string path) {
  DataObjectCollection::path = path;
  DataObjectCollection::nextId = 1;
  DataObjectCollection::objects = {};
}

void DataObjectCollection::load() {
  struct stat stats;

  // Get the file path stats
  if (stat(path.c_str(), &stats) != 0) {
    // File doesn't exist, no loading to be done
    return;
  }

  // Open binary stream to the file
  ifstream stream(path, ios::binary);

  if (!stream.is_open()) {
    throw std::exception(
        "Failed to open stream to data object collection file");
  }

  // Read the nextId from the stream
  stream.read(reinterpret_cast<char*>(&nextId), sizeof(nextId));

  // Handle initial read error
  if (stream.fail()) {
    throw std::exception("Error while reading data object collection nextId");
  }

  // Read the length of the object entries map
  uint32_t size;
  stream.read(reinterpret_cast<char*>(&size), sizeof(size));

  // Reserve the space for the new objects
  objects.reserve(size);

  for (uint32_t i = 0; i < size; i++) {
    // Deserialize a data object from the stream
    DataObject object;
    object.deserialize(stream);

    if (stream.fail()) {
      throw std::exception(
          "Error while reading data object collection objects");
    }

    // Store the loaded object in the collection
    objects.push_back(object);
  }

  // Close the finished stream
  stream.close();
}

void DataObjectCollection::save() const {
  ofstream stream(DataObjectCollection::path.c_str(), ios::binary | ios::trunc);

  if (!stream.is_open()) {
    throw std::exception(
        "Failed to open stream to data object collection file");
  }

  // Write the next ID
  stream.write(reinterpret_cast<const char*>(&nextId), sizeof(nextId));

  // Handle initial write error
  if (stream.fail()) {
    throw std::exception("Error while writing data object collection nextId");
  }

  // Write the size of the object list
  uint32_t size = static_cast<uint32_t>(objects.size());
  stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

  if (stream.fail()) {
    throw std::exception("Failed to write objects size");
  }

  for (DataObject const& object : objects) {
    object.serialize(stream);

    if (stream.fail()) {
      throw std::exception(
          "Error while writing data object collection objects");
    }
  }

  // Close the finished stream
  stream.close();
}

DataObject* DataObjectCollection::getObject(uint32_t id) {
  // Search the objects for a matching ID
  for (size_t i = 0; i < objects.size(); i++) {
    DataObject* object = &objects[i];
    if (object->id == id) {
      return object;
    }
  }

  // No matching object found
  return nullptr;
}

void DataObjectCollection::deleteObject(uint32_t id) {
  // Search the objects for a matching ID
  for (size_t i = 0; i < objects.size(); i++) {
    DataObject* object = &objects[i];
    if (object->id == id) {
      // Remove the object
      objects.erase(objects.begin() + i);
      return;
    }
  }
}

DataObject* DataObjectCollection::createObject() {
  // Get and increment the next ID
  uint32_t id = nextId;
  nextId++;

  // Create the new object
  DataObject object = DataObject();
  object.id = id;

  // Insert the object into the collection
  objects.push_back(object);

  // Get a reference to the inserted object
  DataObject* insertedObject = &objects.back();

  return insertedObject;
}

size_t DataObjectCollection::getObjectCount() {
  return objects.size();
}

DataObject::DataObject() : id(0), entries{} {}

void DataObject::setEntry(string key, DataValue value) {
  entries[key] = value;

  DataObject::entries.insert(std::make_pair(key, value));
}

DataValue* DataObject::getEntry(string key) {
  return &DataObject::entries[key];
}

void serializeString(ofstream& stream, const string& value) {
  // Get the length of the string
  uint32_t length = static_cast<uint32_t>(value.size());
  // Write the length of the string
  stream.write(reinterpret_cast<const char*>(&length), sizeof(length));
  // Write the key string bytes
  stream.write(value.c_str(), length);
}

void deserializeString(ifstream& stream, string& out) {
  // Read the length of the string
  uint32_t length;
  stream.read(reinterpret_cast<char*>(&length), sizeof(length));

  // Resize the output string
  out.resize(length);

  // Read the key contents from the stream
  stream.read(&out[0], length);
}

void DataObject::deserialize(ifstream& stream) {
  // Read the object ID
  stream.read(reinterpret_cast<char*>(&id), sizeof(id));

  // Read the length of the object entries map
  uint32_t size;
  stream.read(reinterpret_cast<char*>(&size), sizeof(size));

  for (uint32_t i = 0; i < size; i++) {
    // Deserialize the entry key
    string key;
    deserializeString(stream, key);

    // Initialize and deserialize an entry
    DataValue entry;
    entry.deserialize(stream);

    if (stream.fail()) {
      throw std::exception("Error while reading data object collection object");
    }

    DataObject::entries[key] = entry;
  }
}

void DataObject::serialize(ofstream& stream) const {
  // Write the object ID
  stream.write(reinterpret_cast<const char*>(&id), sizeof(id));

  // Get the length of the entries map
  uint32_t size = static_cast<uint32_t>(entries.size());

  // Write the entries length
  stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

  for (std::pair<const string&, const DataValue&> entry : DataObject::entries) {
    const string& key = entry.first;
    const DataValue value = entry.second;

    // Serialize the key
    serializeString(stream, key);

    // Serialize the value
    value.serialize(stream);
  }
}

DataValue::DataValue() : type(DataValue::INTEGER), intValue(0) {}

DataValue::DataValue(const DataValue& other) {
  DataValue::type = other.type;

  switch (DataValue::type) {
    case DataValue::STRING: {
      new (&stringValue) std::string(other.stringValue);
      break;
    }
    case DataValue::INTEGER: {
      DataValue::intValue = other.intValue;
      break;
    }
    case DataValue::FLOAT: {
      DataValue::floatValue = other.floatValue;
      break;
    }
    // Fallback default to assign the integer value
    default:
      intValue = 0;
      break;
  }
}

DataValue::~DataValue() {
  switch (DataValue::type) {
    case DataValue::STRING: {
      // Only strings require cleanup here1
      DataValue::stringValue.~basic_string();
      break;
    }
    case DataValue::INTEGER:
    case DataValue::FLOAT:
      // No additional cleanup behavior
      break;
  }
}

DataValue::DataValue(const std::string& value)
    : type(DataValue::STRING), stringValue(value) {}

DataValue::DataValue(int32_t value)
    : type(DataValue::INTEGER), intValue(value) {}

DataValue::DataValue(float value) : type(DataValue::FLOAT), floatValue(value) {}

string* DataValue::asString() {
  if (DataValue::type != DataValue::STRING) {
    return nullptr;
  }
  return &this->stringValue;
}

int32_t* DataValue::asInt() {
  if (DataValue::type != DataValue::INTEGER) {
    return nullptr;
  }
  return &this->intValue;
}

float* DataValue::asFloat() {
  if (DataValue::type != DataValue::FLOAT) {
    return nullptr;
  }
  return &this->floatValue;
}

void DataValue::serialize(ofstream& stream) const {
  // Write the type
  stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

  switch (DataValue::type) {
    case DataValue::STRING: {
      // Serialize the string value
      serializeString(stream, stringValue);

      break;
    }
    case DataValue::INTEGER: {
      stream.write(reinterpret_cast<const char*>(&intValue), sizeof(intValue));
      break;
    }
    case DataValue::FLOAT: {
      stream.write(reinterpret_cast<const char*>(&floatValue),
                   sizeof(floatValue));
      break;
    }
  }
}

void DataValue::deserialize(std::ifstream& stream) {
  // Read the type byte from the stream
  stream.read(reinterpret_cast<char*>(&type), sizeof(type));

  switch (type) {
    case DataValue::STRING: {
      // Initialize the new string value
      new (&stringValue) std::string();

      // Deserialize the string value
      deserializeString(stream, stringValue);

      break;
    }
    case DataValue::INTEGER:
      stream.read(reinterpret_cast<char*>(&intValue), sizeof(intValue));
      break;

    case DataValue::FLOAT:
      stream.read(reinterpret_cast<char*>(&floatValue), sizeof(floatValue));
      break;

    default:
      throw std::runtime_error("Unexpected data entry type");
  }
}

DataValue& DataValue::operator=(const DataValue& other) {
  if (this == &other)
    return *this;

  this->~DataValue();

  DataValue::type = other.type;
  switch (DataValue::type) {
    case DataValue::STRING:
      new (&stringValue) std::string(other.stringValue);
      break;
    case DataValue::INTEGER:
      intValue = other.intValue;
      break;
    case DataValue::FLOAT:
      floatValue = other.floatValue;
      break;
  }

  return *this;
}
