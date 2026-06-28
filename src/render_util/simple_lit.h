#pragma once
// Full Phong lighting shader — ambient(hemisphere) + diffuse + specular + point light + shadow map
// Pure header, depends only on GLSLProgram, Mesh, glm

#include "../base/glsl_program.h"
#include "../mesh/mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class SimpleLitShader {
    std::unique_ptr<GLSLProgram> _prog;
public:
    SimpleLitShader() {
        const char* vs = R"(#version 330 core
            layout(location=0) in vec3 aP; layout(location=1) in vec3 aN;
            out vec3 fn; out vec3 fp; out vec4 fls;
            uniform mat4 uM,uV,uP,uLS;
            void main(){
                vec4 wp=uM*vec4(aP,1);
                fn=mat3(transpose(inverse(uM)))*aN;
                fp=wp.xyz;
                fls=uLS*wp;
                gl_Position=uP*uV*wp;
            })";
        const char* fs = R"(#version 330 core
            in vec3 fn; in vec3 fp; in vec4 fls;
            out vec4 o;
            uniform vec3 uColor,uSunDir,uSunColor,uAmbient,uSpecColor,uCamPos,uPtPos,uPtColor;
            uniform float uAlpha,uShininess,uPtIntensity,uPtAttenConst,uPtAttenLinear,uPtAttenQuad,uShadowBias;
            uniform sampler2D uShadowMap;
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

                // === Hemisphere ambient (bottom faces darker) ===
                float hemi=0.5+0.5*N.y;
                vec3 ambient=uAmbient*uColor*hemi;

                // === Directional sun ===
                vec3 L=normalize(-uSunDir);
                float nd=max(dot(N,L),0.);
                vec3 Rf=reflect(-L,N);
                float ns=pow(max(dot(Rf,V),0.),uShininess);
                vec3 diffuse=uSunColor*uColor*nd;
                vec3 specular=uSunColor*uSpecColor*ns;

                // === Point light with attenuation ===
                vec3 ptL=uPtPos-fp;
                float ptDist=length(ptL);
                vec3 ptDir=ptL/max(ptDist,0.001);
                float atten=uPtIntensity/(uPtAttenConst+uPtAttenLinear*ptDist+uPtAttenQuad*ptDist*ptDist);
                float ptNd=max(dot(N,ptDir),0.);
                vec3 ptR=reflect(-ptDir,N);
                float ptNs=pow(max(dot(ptR,V),0.),uShininess);
                vec3 ptContrib=atten*uPtColor*(uColor*ptNd+uSpecColor*ptNs);

                float sh=shadowFactor(fls);
                vec3 lit=ambient+(diffuse+specular)*sh+ptContrib;
                vec3 gamma=pow(lit,vec3(1.0/2.2));
                o=vec4(gamma,uAlpha);
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

// ============================================================
// Draw helpers — full Phong + shadow params
// ============================================================
struct PhongParams {
    glm::vec3 sunDir       = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    glm::vec3 sunColor     = glm::vec3(1.0f, 0.95f, 0.85f);
    glm::vec3 ambient      = glm::vec3(0.3f);
    glm::vec3 specColor    = glm::vec3(1.0f);
    float     shininess    = 32.0f;
    glm::vec3 camPos       = glm::vec3(0, 5, 20);
    glm::vec3 ptPos        = glm::vec3(3, 4, -2);
    glm::vec3 ptColor      = glm::vec3(0.3f, 0.5f, 1.0f);
    float     ptIntensity  = 5.0f;
    float     ptAttenConst = 1.0f;
    float     ptAttenLinear = 0.09f;
    float     ptAttenQuad  = 0.032f;
    glm::mat4 lightSpace   = glm::mat4(1.0f);
    GLuint    shadowMap    = 0;
    float     shadowBias   = 0.0005f;
    bool      shadowsOn    = true;
};

inline void drawLit(SimpleLitShader& shader,
                     Mesh* mesh,
                     const glm::mat4& model,
                     const glm::vec3& color,
                     const glm::mat4& proj,
                     const glm::mat4& view,
                     const PhongParams& pp = PhongParams())
{
    shader->use();
    shader->setUniformMat4("uP", proj);
    shader->setUniformMat4("uV", view);
    shader->setUniformMat4("uM", model);
    shader->setUniformMat4("uLS", pp.lightSpace);
    shader->setUniformVec3("uColor", color);
    shader->setUniformVec3("uSunDir", pp.sunDir);
    shader->setUniformVec3("uSunColor", pp.sunColor);
    shader->setUniformVec3("uAmbient", pp.ambient);
    shader->setUniformVec3("uSpecColor", pp.specColor);
    shader->setUniformFloat("uShininess", pp.shininess);
    shader->setUniformVec3("uCamPos", pp.camPos);
    shader->setUniformVec3("uPtPos", pp.ptPos);
    shader->setUniformVec3("uPtColor", pp.ptColor);
    shader->setUniformFloat("uPtIntensity", pp.ptIntensity);
    shader->setUniformFloat("uPtAttenConst", pp.ptAttenConst);
    shader->setUniformFloat("uPtAttenLinear", pp.ptAttenLinear);
    shader->setUniformFloat("uPtAttenQuad", pp.ptAttenQuad);
    shader->setUniformFloat("uShadowBias", pp.shadowBias);
    shader->setUniformBool("uShadowsOn", pp.shadowsOn);
    shader->setUniformFloat("uAlpha", 1.0f);
    if (pp.shadowMap) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pp.shadowMap);
        shader->setUniformInt("uShadowMap", 1);
    } else {
        shader->setUniformInt("uShadowMap", 0);
    }
    mesh->draw();
}

inline void drawLit(SimpleLitShader& shader,
                     Mesh* mesh,
                     const glm::mat4& model,
                     const glm::vec4& color,
                     const glm::mat4& proj,
                     const glm::mat4& view,
                     const PhongParams& pp = PhongParams())
{
    shader->use();
    shader->setUniformMat4("uP", proj);
    shader->setUniformMat4("uV", view);
    shader->setUniformMat4("uM", model);
    shader->setUniformMat4("uLS", pp.lightSpace);
    shader->setUniformVec3("uColor", glm::vec3(color));
    shader->setUniformVec3("uSunDir", pp.sunDir);
    shader->setUniformVec3("uSunColor", pp.sunColor);
    shader->setUniformVec3("uAmbient", pp.ambient);
    shader->setUniformVec3("uSpecColor", pp.specColor);
    shader->setUniformFloat("uShininess", pp.shininess);
    shader->setUniformVec3("uCamPos", pp.camPos);
    shader->setUniformVec3("uPtPos", pp.ptPos);
    shader->setUniformVec3("uPtColor", pp.ptColor);
    shader->setUniformFloat("uPtIntensity", pp.ptIntensity);
    shader->setUniformFloat("uPtAttenConst", pp.ptAttenConst);
    shader->setUniformFloat("uPtAttenLinear", pp.ptAttenLinear);
    shader->setUniformFloat("uPtAttenQuad", pp.ptAttenQuad);
    shader->setUniformFloat("uShadowBias", pp.shadowBias);
    shader->setUniformBool("uShadowsOn", pp.shadowsOn);
    if (pp.shadowMap) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pp.shadowMap);
        shader->setUniformInt("uShadowMap", 1);
    } else {
        shader->setUniformInt("uShadowMap", 0);
    }
    shader->setUniformFloat("uAlpha", color.a);
    mesh->draw();
}