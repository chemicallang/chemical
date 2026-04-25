public interface HtmlPageWriter {

    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue;

    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderHtmlAttrValue(page, getSsrAttributeValue(page))
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderJsAttrValue(page, getSsrAttributeValue(page))
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderCssAttrValue(page, getSsrAttributeValue(page))
    }

}

impl HtmlPageWriter for std::string_view {
    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue {
        return SsrAttributeValue.Text(SsrText { data : self.data(), size : self.size() });
    }
    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append_with_len(self.data(), self.size())
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        buffer.append_with_len(self.data(), self.size())
        buffer.append('\'');
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        buffer.append_with_len(self.data(), self.size())
        buffer.append('\'');
    }
}

impl HtmlPageWriter for std::string {
    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue {
        return SsrAttributeValue.Text(SsrText { data : self.data(), size : self.size() });
    }
    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append_with_len(self.data(), self.size())
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        buffer.append_with_len(self.data(), self.size())
        buffer.append('\'');
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        buffer.append_with_len(self.data(), self.size())
        buffer.append('\'');
    }
}