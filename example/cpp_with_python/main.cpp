#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>

#include <cstdio>
#include <iostream>
#include <string>
std::string python_path = PYTHON_PATH;


int main(int argc, char *argv[]) {
    setenv("PYTHONPATH", python_path.c_str(), 1);
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    Py_Initialize();
    pName   = PyUnicode_DecodeFSDefault("machine_learning");
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "predict");
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(5);
            for (i = 0; i < 5; ++i) {
                pValue = PyFloat_FromDouble(123.312);
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
                    return 1;
                }
                PyTuple_SetItem(pArgs, i, pValue);
            }
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                PyObject *key, *value;
                Py_ssize_t pos = 0;
                while (PyDict_Next(pValue, &pos, &key, &value)) {
                    PyObject *key_as_str =
                        PyUnicode_AsEncodedString(key, "utf-8", "~E~");
                    const char *key_local = PyBytes_AS_STRING(key_as_str);
                    long value_local = PyLong_AS_LONG(value);
                    std::cout << "key = " << key_local
                              << " value = " << value_local << std::endl;
                    Py_XDECREF(key_as_str);
                }
                Py_DECREF(pValue);
            } else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr, "Call failed\n");
                return 1;
            }
        } else {
            if (PyErr_Occurred()) PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", argv[2]);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", argv[1]);
        return 1;
    }
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
    return 0;
}