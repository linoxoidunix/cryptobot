#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <string_view>
#include <iostream>
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
        predictor_class_ = PyDict_GetItemString(module_dict_, CLASS_PREDICTOR_NAME.data());
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
};
};  // namespace detail
