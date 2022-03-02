#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <errno.h>

#include "src/data_plane/interfaces/intrusive/imonarch.h"
#include "src/data_plane/logic/remote/bootstrap_client.h"
#include "src/control_plane/control_application.h"
#include "src/data_plane/logic/remote/pytorch_multiprocess/us_client.h"
#include "src/data_plane/logic/remote/pytorch_multiprocess/us_server.h"

namespace py = pybind11;

PYBIND11_MODULE(_py_pastor, m)
{
	m.doc() = "pastor";

	py::class_<ControlApplication>(m, "ControlApplication")
        .def(py::init<>())
        .def("run", &ControlApplication::run);

#if defined(INCLUDE_GRPC)
	py::class_<BootstrapClient>(m, "BootstrapClient")
        .def(py::init<const std::string&>())
        .def("request_session", &BootstrapClient::request_session)
        .def("simple_request_session", &BootstrapClient::simple_request_session)
        .def("get_group", &BootstrapClient::get_group)
        .def("get_data_source_infos", &BootstrapClient::get_data_source_infos)
        .def("get_ids", &BootstrapClient::get_ids)
        .def("get_ids_from_rank", &BootstrapClient::get_ids_from_rank)
        .def("get_filenames", &BootstrapClient::get_filenames)
        .def("get_filenames_full_path", &BootstrapClient::get_filenames_full_path);
#endif

	py::class_<USServer>(m, "USServer")
	    .def(py::init<>())
	    .def("start", &USServer::start);

	py::class_<USClient>(m, "USClient")
        .def(py::init<>())
#if defined(INCLUDE_GRPC)
        .def("bind", &USClient::bind)
#endif
        .def("read", [](USClient& cli, int id){
	        size_t n = cli.prepare_transfer(id);
            int target = cli.get_target();
            PyBytesObject* bytesObject = (PyBytesObject*) PyObject_Malloc(offsetof(PyBytesObject, ob_sval) + n);
            PyObject_INIT_VAR(bytesObject, &PyBytes_Type, n);
            bytesObject->ob_shash = -1;
	        cli.read(bytesObject->ob_sval);
	        return std::make_pair(target, py::reinterpret_steal<py::object>((PyObject*)bytesObject));
        }, py::return_value_policy::take_ownership);

	py::class_<IMonarch>(m, "Monarch")
		.def(py::init<int, int>())
#if defined(INCLUDE_GRPC)
		.def("server_attach_init", &IMonarch::server_attach_init)
#endif
		.def("standalone_init", &IMonarch::standalone_init)
		.def("start", &IMonarch::start)
		.def("read", [](IMonarch& dpi, int id) {
			int n = dpi.get_file_size_from_id(id);
			PyBytesObject* bytesObject = (PyBytesObject*) PyObject_Malloc(offsetof(PyBytesObject, ob_sval) + n);
			PyObject_INIT_VAR(bytesObject, &PyBytes_Type, n);
			bytesObject->ob_shash = -1;
			dpi.read_from_id(id, bytesObject->ob_sval);
			int target = dpi.get_target_class_from_id(id);
			return std::make_pair(target, py::reinterpret_steal<py::object>((PyObject*)bytesObject));
		}, py::return_value_policy::take_ownership);
}
