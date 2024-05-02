#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>
#include <string_view>
namespace detail {
std::string python_path                = PYTHON_PATH;
std::string_view MODULE_PREDICTOR_NAME = "machine_learning";
std::string_view CLASS_PREDICTOR_NAME  = "Predictor";
std::string_view METHOD_PREDICTOR_NAME = "predict";

class PredictorImpl {
  public:
    explicit PredictorImpl(uint window_size) : window_size_(window_size) {
        setenv("PYTHONPATH", python_path.c_str(), 1);
    };
    /**
     * @brief init inner variable
     *
     */
    int Init() {
        Py_Initialize();
        module_name_ = PyUnicode_DecodeFSDefault(MODULE_PREDICTOR_NAME.data());
        module_      = PyImport_Import(module_name_);
        Py_XDECREF(module_name_);
        if (module_ == nullptr) {
            PyErr_Print();
            std::cerr << "Fails to import the module.\n";
            return 1;
        }
        // dict is a borrowed reference.
        module_dict_ = PyModule_GetDict(module_);
        if (module_dict_ == nullptr) {
            PyErr_Print();
            std::cerr << "Fails to get the dictionary.\n";
            return 2;
        }
        Py_XDECREF(module_);
        // Builds the name of a callable class
        predictor_class_ =
            PyDict_GetItemString(module_dict_, CLASS_PREDICTOR_NAME.data());
        if (predictor_class_ == nullptr) {
            PyErr_Print();
            std::cerr << "Fails to get the Python class.\n";
            return 3;
        }
        Py_XDECREF(module_dict_);

        // Creates an instance of the class
        if (PyCallable_Check(predictor_class_)) {
            predictor_ = PyObject_CallObject(predictor_class_, nullptr);
            Py_XDECREF(predictor_class_);
        } else {
            std::cout << "Cannot instantiate the Python class" << std::endl;
            Py_XDECREF(predictor_class_);
            return 4;
        }
        return 0;
    }
    std::pair<std::string, long> Predict(double open, double high, double low,
                                         double close, double volume) {
        auto pArgs = PyTuple_New(5);
        if (InitTuplePosition(pArgs, 0, open) ||
            InitTuplePosition(pArgs, 1, high) ||
            InitTuplePosition(pArgs, 2, low) ||
            InitTuplePosition(pArgs, 3, close) ||
            InitTuplePosition(pArgs, 4, volume)) {
            Py_DECREF(pArgs);
            fprintf(stderr, "Cannot convert argument\n");
            return {};
        }
        PyObject *value =
            PyObject_CallMethod(predictor_, METHOD_PREDICTOR_NAME.data(),
                                "(ddddd)", open, high, low, close, volume);
        Py_DECREF(pArgs);
        PyObject *key1, *value1;
        Py_ssize_t pos1 = 0;
        // while (PyDict_Next(value, &pos1, &key1, &value1)) {
        //     int x = 0;
        // }
        std::string key_out;
        long value_out;
        auto sdfsdf = PyDict_Size(value);
        if (value != NULL && PyDict_CheckExact(value)) {
            PyObject *key2, *value2;
            Py_ssize_t pos2 = 0;
            while (PyDict_Next(value, &pos2, &key2, &value2)) {
                PyObject *key_as_str =
                    PyUnicode_AsEncodedString(key2, "utf-8", "~E~");
                const char *key_local = PyBytes_AS_STRING(key_as_str);
                long value_local      = PyLong_AS_LONG(value2);
                //std::cout << "key = " << key_local << " value = " << value_local
                //          << std::endl;
                key_out   = std::string(key_local);
                value_out = value_local;
                Py_XDECREF(key_as_str);
            }
            Py_DECREF(value);
        }
        return std::make_pair(key_out, value_out);
    }
    ~PredictorImpl() {
        Py_XDECREF(module_name_);
        Py_XDECREF(module_);
        Py_XDECREF(module_dict_);
        Py_XDECREF(predictor_class_);
        Py_Finalize();
    };

  private:
    uint window_size_;
    PyObject *module_name_, *module_, *module_dict_, *predictor_class_;
    /**
     * @brief instance of Python class
     *
     */
    PyObject *predictor_;

  private:
    int InitTuplePosition(PyObject *pArgs, int position, double value) {
        auto pValue = PyFloat_FromDouble(value);
        if (!pValue) {
            fprintf(stderr, "Cannot convert argument\n");
            return 1;
        }
        PyTuple_SetItem(pArgs, position, pValue);

        return 0;
    }
};
};  // namespace detail
