#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdlib.h>

#include <boost/filesystem/path.hpp>
#include <cstdio>
#include <string>
#include <string_view>

#include "aot/Logger.h"
#include "aot/common/types.h"

class IStrategy {
  public:
    /**
     * @brief Action can be "enter_long", "enter_short", "exit_short"
     *
     */
    using Action     = std::string;
    using Confidence = long;
    struct Result {
        Action action;
        Confidence confidence;
    };
    virtual Result Predict(double open, double high, double low, double close,
                           double volume) = 0;
    virtual ~IStrategy()                  = default;
};

namespace base_strategy {
class Strategy : public IStrategy {
  public:
    /**
     * @brief Construct a new Launcher Predictor object
     *
     * @param python_path PYTHONPATH environment variable, path where need
     * search python modules
     * @param file_predictor name file where exist python class predictor
     * @param class_predictor name python predictor class that has predict(open,
     * high, low, close, volume) method
     * @param method_predictor name python predictor method of predictor python
     * class
     */
    explicit Strategy(std::string_view python_path,
                      std::string_view path_where_models,
                      std::string_view file_predictor,
                      std::string_view class_predictor,
                      std::string_view method_predictor)
        : file_predictor_(file_predictor.data()),
          class_predictor_(class_predictor.data()),
          method_predictor_(method_predictor.data()) {
        setenv("PYTHONPATH", python_path.data(), 1);
        Py_Initialize();
        //PyRun_SimpleString("import sys\nprint(sys.path)");
        size_t lastindex                 = file_predictor_.find_last_of(".");
        auto file_name_without_extension = file_predictor_.substr(0, lastindex);
        file_name_ =
            PyUnicode_DecodeFSDefault(file_name_without_extension.c_str());
        module_name_ =
            PyImport_Import(file_name_);  // get filename without extension
        Py_DECREF(file_name_);
        class_name_ =
            PyObject_GetAttrString(module_name_, class_predictor_.c_str());
        Py_DECREF(module_name_);

        auto args = PyTuple_New(1);
        InitTuplePosition(args, 0, path_where_models.data());

        predictor_instance_ = PyObject_CallObject(class_name_, args);
        Py_XDECREF(args);
        Py_XDECREF(class_name_);
    };
    IStrategy::Result Predict(double open, double high, double low,
                              double close, double volume) override {
        IStrategy::Result result;

        pValue =
            PyObject_CallMethod(predictor_instance_, method_predictor_.c_str(),
                                "(fffff)", open, high, low, close, volume);

        if (pValue != NULL) {
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            while (PyDict_Next(pValue, &pos, &key, &value)) {
                PyObject *key_as_str =
                    PyUnicode_AsEncodedString(key, "utf-8", "~E~");
                const char *key_local = PyBytes_AS_STRING(key_as_str);
                long value_local      = PyLong_AS_LONG(value);
                result = {key_local, value_local};
                Py_XDECREF(key_as_str);
            }
            Py_DECREF(pValue);
        } else {
            PyErr_Print();
            logi("pValue = NULL. Call failed");
            return result;
        }
        return result;
    };
    ~Strategy() override { Py_DECREF(predictor_instance_); Py_Finalize();};
    class Parser {
      public:
        explicit Parser() = default;
        common::TradeAction Parse(const IStrategy::Result &result) const {
            // base on "aot/python/my_types.py"
            if (result.action == "enter_long")
                return common::TradeAction::kEnterLong;
            if (result.action == "enter_short")
                return common::TradeAction::kEnterShort;
            if (result.action == "exit_long")
                return common::TradeAction::kExitLong;
            if (result.action == "exit_short")
                return common::TradeAction::kExitShort;
            if (result.action == "") return common::TradeAction::kNope;
            return common::TradeAction::kNope;
        };
    };

  private:
    std::string file_predictor_;
    std::string class_predictor_;
    std::string method_predictor_;
    PyObject *file_name_;
    PyObject *module_name_;
    PyObject *class_name_;
    PyObject *predictor_instance_;
    PyObject *pValue;
    int InitTuplePosition(PyObject *pArgs, int position, double value) {
        auto obj_value = PyFloat_FromDouble(value);
        if (!obj_value) {
            fprintf(stderr, "Cannot convert argument\n");
            return 1;
        }
        PyTuple_SetItem(pArgs, position, obj_value);

        return 0;
    };
    int InitTuplePosition(PyObject *pArgs, int position, const char *value) {
        auto obj_value = PyUnicode_FromString(value);
        if (!obj_value) {
            fprintf(stderr, "Cannot convert argument\n");
            return 1;
        }
        PyTuple_SetItem(pArgs, position, obj_value);

        return 0;
    }
};
}  // namespace base_strategy
