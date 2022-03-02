#include "tests/test_class.h"

#include <vector>

int main() {
    auto *tc = new TestClass();
    tc->run_intrusive_instance("/home/dantas/storage_backend_test/lustre/tf_data", 3, true, false);
    //tc->run_transparent("/home/dantas/storage_backend_test/lustre/hymenoptera_data", 2, true);
}