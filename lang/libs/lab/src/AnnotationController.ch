@compiler.interface
public struct AnnotationController {

    func createSingleMarkerAnnotation(&self, name : &std::string_view, policy : int)

    func createMarkerAnnotation(&self, name : &std::string_view)

    func createCollectorAnnotation(&self, name : &std::string_view, expected_usage : uint)

    func createMarkerAndCollectorAnnotation(&self, name : &std::string_view, expected_usage : uint)

}