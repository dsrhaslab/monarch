//
// Created by dantas on 10/04/22.
//

#ifndef MONARCH_INTRUSIVE_STAGE_IMPL_H
#define MONARCH_INTRUSIVE_STAGE_IMPL_H

ssize_t HierarchicalStage::read(const std::string& filename, char* result, off_t offset, size_t n) {
    auto* fi = metadata_container->get_metadata(filename);
    return read(fi, result, offset, n, false);
}

ssize_t HierarchicalStage::read_from_id(int file_id, char* result, off_t offset, size_t n) {
    auto* fi = metadata_container->get_metadata(file_id);
    return read(fi, result, offset, n, false);
}

ssize_t HierarchicalStage::read_from_id(int file_id, char* result){
    auto* fi = metadata_container->get_metadata(file_id);
    return read(fi, result, 0, fi->_get_size(), false);
}

size_t HierarchicalStage::get_file_size_from_id(int id){
    return metadata_container->get_metadata(id)->_get_size();
}

int HierarchicalStage::get_target_class(const std::string &filename){
    return metadata_container->get_metadata(filename)->get_target();
}

int HierarchicalStage::get_target_class_from_id(int id) {
    return metadata_container->get_metadata(id)->get_target();
}

#endif //MONARCH_INTRUSIVE_STAGE_IMPL_H
