#pragma once
// Textured Phong shader — ambient(hemisphere) + diffuse + specular + point + shadow map
// Supports per-face tile selection from a texture atlas via vertex normal

#include "../base/glsl_program.h"
#include "../mesh/mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class TexturedLitShader {
    std::unique_ptr<GLSLProgram> _prog;
public:
    TexturedLitShader() {
        const char* vs = R"(#version 330 core
            layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec2 aUV;
            out vec3 fn; out vec2 fuv; out vec3 fp; out vec4 fls;
            uniform mat4 uM,uV,uP,uLS;
            void main(){
                vec4 wp=uM*vec4(aP,1);
                fn=mat3(transpose(inverse(uM)))*aN;
                fuv=aUV;
                fp=wp.xyz;
                fls=uLS*wp;
                gl_Position=uP*uV*wp;
            })";
        const char* fs = R"(#version 330 core
            in vec3 fn; in vec2 fuv; in vec3 fp; in vec4 fls;
            out vec4 o;
            uniform sampler2D uTex;
            uniform sampler2D uShadowMap;
            uniform vec3 uSunDir,uSunColor,uAmbient,uSpecColor,uCamPos,uPtPos,uPtColor;
            uniform float uShininess,uPtIntensity,uPtAttenConst,uPtAttenLinear,uPtAttenQuad,uShadowBias;
            uniform int uBaseTile;
            uniform float uAtlasCols;
            uniform bool uShadowsOn;

            float shadowFactor(vec4 lsPos){
                if(!uShadowsOn) return 1.0;
                vec3 proj=lsPos.xyz/lsPos.w;
                proj=proj*0.5+0.5;
                if(proj.x<0.0||proj.x>1.0||proj.y<0.0||proj.y>1.0||proj.z>1.0) return 1.0;
                float currentDepth=proj.z;
                float bias=uShadowBias;
                float sum=0.0;
                vec2 ts=1.0/textureSize(uShadowMap,0);
                for(int dx=-2;dx<=2;dx++)
                for(int dy=-2;dy<=2;dy++){
                    float closestDepth=texture(uShadowMap,proj.xy+vec2(dx,dy)*ts).r;
                    sum+=(currentDepth-bias>closestDepth)?0.0:1.0;
                }
                return sum/25.0;
            }

            void main(){
                vec3 N=normalize(fn);
                vec3 V=normalize(uCamPos-fp);

                // === Select tile based on normal ===
                int tile=uBaseTile;
                if(abs(N.y)>0.9){
                    tile+=(N.y>0.0)?0:1;
                }else{
                    tile+=2;
                }
                float row=floor(float(tile)/uAtlasCols);
                float col=float(tile)-row*uAtlasCols;
                float ts=1.0/uAtlasCols;
                vec2 uv=(fuv*ts)+vec2(col*ts,row*ts);
                vec4 tex=texture(uTex,uv);

                // === Hemisphere ambient ===
                float hemi=0.5+0.5*N.y;
                vec3 ambient=uAmbient*tex.rgb*hemi;

                // === Directional sun ===
                vec3 L=normalize(-uSunDir);
                float nd=max(dot(N,L),0.);
                vec3 Rf=reflect(-L,N);
                float ns=pow(max(dot(Rf,V),0.),uShininess);
                vec3 diffuse=uSunColor*tex.rgb*nd;
                vec3 specular=uSunColor*uSpecColor*ns;

                // === Point light ===
                vec3 ptL=uPtPos-fp;
                float ptDist=length(ptL);
                vec3 ptDir=ptL/max(ptDist,0.001);
                float atten=uPtIntensity/(uPtAttenConst+uPtAttenLinear*ptDist+uPtAttenQuad*ptDist*ptDist);
                float ptNd=max(dot(N,ptDir),0.);
                vec3 ptR=reflect(-ptDir,N);
                float ptNs=pow(max(dot(ptR,V),0.),uShininess);
                vec3 ptContrib=atten*uPtColor*(tex.rgb*ptNd+uSpecColor*ptNs);

                // === Shadow ===
                float sh=shadowFactor(fls);
                vec3 lit=ambient+(diffuse+specular)*sh+ptContrib;
                o=vec4(lit,tex.a);
            })";
        _prog = std::make_unique<GLSLProgram>();
        _prog->attachVertexShader(vs);
        _prog->attachFragmentShader(fs);
        _prog->link();
    }

    GLSLProgram* operator->() { return _prog.get(); }
    GLSLProgram* get() { return _prog.get(); }
    void use() { _prog->use(); }
};

// One-call textured draw with full Phong + shadow params
inline void drawTexturedLit(TexturedLitShader& shader,
                             Mesh* mesh,
                             const glm::mat4& model,
                             int baseTile,
                             float atlasCols,
                             GLuint texId,
                             const glm::mat4& proj,
                             const glm::mat4& view,
                             const PhongParams& pp = PhongParams())
{
    shader->use();
    shader->setUniformMat4("uP", proj);
    shader->setUniformMat4("uV", view);
    shader->setUniformMat4("uM", model);
    shader->setUniformMat4("uLS", pp.lightSpace);
    shader->setUniformInt("uBaseTile", baseTile);
    shader->setUniformFloat("uAtlasCols", atlasCols);
    shader->setUniformVec3("uSunDir", pp.sunDir);
    shader->setUniformVec3("uSunColor", pp.sunColor);
    shader->setUniformVec3("uAmbient", pp.ambient);
    shader->setUniformVec3("uSpecColor", pp.specColor);
    shader->setUniformFloat("uShininess", pp.shininess);
    shader->setUniformVec3("uCamPos", pp.camPos);
    shader->setUniformVec3("uPtPos", pp.ptPos);
    shader->setUniformVec3("uPtColor", pp.ptColor);
    shader->setUniformFloat("uShadowBias", pp.shadowBias);
    shader->setUniformBool("uShadowsOn", pp.shadowsOn);
    shader->setUniformFloat("uPtIntensity", pp.ptIntensity);
    shader->setUniformFloat("uPtAttenConst", pp.ptAttenConst);
    shader->setUniformFloat("uPtAttenLinear", pp.ptAttenLinear);
    shader->setUniformFloat("uPtAttenQuad", pp.ptAttenQuad);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    shader->setUniformInt("uTex", 0);
    if (pp.shadowMap) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pp.shadowMap);
        shader->setUniformInt("uShadowMap", 1);
    } else {
        shader->setUniformInt("uShadowMap", 0);
    }
    mesh->draw();
}