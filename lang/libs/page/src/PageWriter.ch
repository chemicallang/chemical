public interface HtmlPageWriter {

    func writeToPageHead(&mut self, page : &mut HtmlPage);

    func writeToPageBody(&mut self, page : &mut HtmlPage);

}

impl HtmlPageWriter for std::string_view {
    func writeToPageHead(&mut self, page : &mut HtmlPage) {
        page.append_head(self.data(), self.size())
    }
    func writeToPageBody(&mut self, page : &mut HtmlPage) {
        page.append_html(self.data(), self.size())
    }
}

impl HtmlPageWriter for std::string {
    func writeToPageHead(&mut self, page : &mut HtmlPage) {
        page.append_head(self.data(), self.size())
    }
    func writeToPageBody(&mut self, page : &mut HtmlPage) {
        page.append_html(self.data(), self.size())
    }
}